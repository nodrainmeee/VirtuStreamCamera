// Test de charge : 1 écrivain @ ~1000 img/s, 3 lecteurs. Toute image lue doit être homogène
// (aucune déchirure) et les numéros de séquence strictement croissants.
#include "vs_video_ring.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
#ifdef _MSC_VER
#include <malloc.h>
static void* vs_alloc(size_t n) { return _aligned_malloc((n + 63) & ~size_t(63), 64); }
static void vs_free(void* p) { _aligned_free(p); }
#else
static void* vs_alloc(size_t n) { return std::aligned_alloc(64, (n + 63) & ~size_t(63)); }
static void vs_free(void* p) { std::free(p); }
#endif
using namespace vs;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

int main() {
    const uint32_t W = 1280, H = 720;
    size_t bytes = RingBytes(PixelFormat::NV12, W, H, W);
    void* mem = vs_alloc(bytes);
    Writer w;
    CHECK(!w.Init(mem, 10, PixelFormat::NV12, W, H, W));          // buffer trop petit
    CHECK(!w.Init(mem, bytes, PixelFormat::NV12, 1281, H, W));    // largeur impaire
    CHECK(w.Init(mem, bytes, PixelFormat::NV12, W, H, W));
    CHECK(w.Header()->payloadBytes == W * H * 3 / 2);

    // Validation hostile côté lecteur
    { Reader r; CHECK(!r.Attach(mem, 16));
      void* e = vs_alloc(bytes);
      std::memcpy(e, mem, bytes);
      reinterpret_cast<RingHeader*>(e)->payloadBytes = 0xFFFFFFF0u;
      CHECK(!r.Attach(e, bytes));
      reinterpret_cast<RingHeader*>(e)->payloadBytes = w.Header()->payloadBytes;
      reinterpret_cast<RingHeader*>(e)->abiVersion = 99;
      CHECK(!r.Attach(e, bytes));
      CHECK(!r.Attach(mem, bytes - 1)); // segment tronqué
      vs_free(e); }

    Reader probe; CHECK(probe.Attach(mem, bytes));
    std::vector<uint8_t> buf(w.Header()->payloadBytes);
    FrameInfo fi{};
    CHECK(probe.TryRead(buf.data(), buf.size(), fi, 0) == ReadResult::NoNewFrame);

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> torn{0}, bad{0}, okc[3]{};
    std::vector<std::thread> readers;
    for (int t = 0; t < 3; ++t) readers.emplace_back([&, t] {
        Reader r; if (!r.Attach(mem, bytes)) { bad++; return; }
        std::vector<uint8_t> b(r.Header()->payloadBytes);
        uint64_t last = 0; FrameInfo f{};
        while (!stop.load()) {
            auto res = r.TryRead(b.data(), b.size(), f, last);
            if (res == ReadResult::Ok) {
                uint8_t v = uint8_t(f.seq & 0xFF);
                for (size_t i = 0; i < b.size(); i += 4096) if (b[i] != v) { torn++; break; }
                if (b.back() != v || f.pts100ns != int64_t(f.seq) * 333333) torn++;
                if (f.seq <= last) bad++;
                last = f.seq; okc[t]++;
            }
        }
    });
    std::vector<uint8_t> frame(w.Header()->payloadBytes);
    const uint64_t N = 3000;
    for (uint64_t i = 1; i <= N; ++i) {
        std::memset(frame.data(), int(i & 0xFF), frame.size());
        CHECK(w.Publish(frame.data(), frame.size(), int64_t(i), int64_t(i) * 333333));
        std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    w.Close();
    CHECK(probe.TryRead(buf.data(), buf.size(), fi, N) == ReadResult::WriterGone);
    stop = true; for (auto& t : readers) t.join();

    std::printf("published=%llu torn=%llu bad=%llu readers_ok=%llu/%llu/%llu\n",
        (unsigned long long)N, (unsigned long long)torn.load(), (unsigned long long)bad.load(),
        (unsigned long long)okc[0].load(), (unsigned long long)okc[1].load(), (unsigned long long)okc[2].load());
    CHECK(torn == 0); CHECK(bad == 0);
    CHECK(okc[0] > 100 && okc[1] > 100 && okc[2] > 100);
    vs_free(mem);
    std::printf(fails ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", fails);
    return fails ? 1 : 0;
}
