// VirtuStream Camera - Video frame ring, ABI v1 (voir shared/FRAME_TRANSPORT_V1.md)
// Header-only, portable : opère sur un bloc mémoire fourni par l'appelant
// (file mapping Windows en production, malloc dans les tests).
// 1 écrivain, N lecteurs en lecture seule. 3 slots, protocole seqlock par slot.
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace vs {

constexpr uint32_t kRingMagic   = 0x52465356u; // 'VSFR'
constexpr uint32_t kAbiVersion  = 1;
constexpr uint32_t kSlotCount   = 3;
constexpr uint32_t kMaxPayload  = 1920u * 1080u * 4u; // borne dure (BGRA 1080p)

enum class PixelFormat : uint32_t { NV12 = 1, BGRA32 = 2 };

struct RingHeader {
    uint32_t magic;
    uint32_t abiVersion;
    uint32_t headerBytes;      // sizeof(RingHeader), arrondi 64
    uint32_t slotCount;        // == kSlotCount
    uint32_t width, height;
    uint32_t format;           // PixelFormat
    uint32_t strideY;          // stride du plan 0 (octets)
    uint32_t payloadBytes;     // octets utiles par image
    uint32_t slotBytes;        // pas entre slots (SlotHeader + payload, aligné 64)
    std::atomic<uint64_t> publishedSeq; // dernier numéro de frame publié (0 = aucun)
    std::atomic<uint32_t> latestSlot;
    std::atomic<uint32_t> writerAlive;  // 1 tant que l'app écrit
};

struct SlotHeader {
    std::atomic<uint64_t> lock;  // impair = écriture en cours
    uint64_t frameSeq;
    int64_t  qpc;                // QueryPerformanceCounter
    int64_t  pts100ns;           // temps de présentation, unités 100 ns
};

struct FrameInfo { uint64_t seq; int64_t qpc; int64_t pts100ns; };

inline constexpr size_t Align64(size_t v) { return (v + 63u) & ~size_t(63); }

inline size_t PayloadBytes(PixelFormat f, uint32_t /*w*/, uint32_t h, uint32_t strideY) {
    return f == PixelFormat::NV12 ? size_t(strideY) * h * 3 / 2
                                  : size_t(strideY) * h;
}

inline size_t RingBytes(PixelFormat f, uint32_t w, uint32_t h, uint32_t strideY) {
    size_t slot = Align64(sizeof(SlotHeader) + PayloadBytes(f, w, h, strideY));
    return Align64(sizeof(RingHeader)) + slot * kSlotCount;
}

class Writer {
public:
    // mem doit avoir au moins RingBytes() octets, aligné 64, initialement quelconque.
    bool Init(void* mem, size_t memBytes, PixelFormat f, uint32_t w, uint32_t h, uint32_t strideY) {
        if (!mem || w == 0 || h == 0 || (w & 1) || (h & 1)) return false;
        if ((reinterpret_cast<uintptr_t>(mem) & 63u) != 0) return false;
        const size_t payload = PayloadBytes(f, w, h, strideY);
        if (strideY < (f == PixelFormat::NV12 ? w : w * 4u)) return false;
        if (payload == 0 || payload > kMaxPayload || memBytes < RingBytes(f, w, h, strideY)) return false;
        std::memset(mem, 0, RingBytes(f, w, h, strideY));
        base_ = static_cast<uint8_t*>(mem);
        hdr_ = reinterpret_cast<RingHeader*>(base_);
        hdr_->abiVersion = kAbiVersion;
        hdr_->headerBytes = static_cast<uint32_t>(Align64(sizeof(RingHeader)));
        hdr_->slotCount = kSlotCount;
        hdr_->width = w; hdr_->height = h;
        hdr_->format = static_cast<uint32_t>(f);
        hdr_->strideY = strideY;
        hdr_->payloadBytes = static_cast<uint32_t>(payload);
        hdr_->slotBytes = static_cast<uint32_t>(Align64(sizeof(SlotHeader) + payload));
        hdr_->publishedSeq.store(0, std::memory_order_relaxed);
        hdr_->latestSlot.store(0, std::memory_order_relaxed);
        hdr_->writerAlive.store(1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        hdr_->magic = kRingMagic; // publié en dernier
        return true;
    }

    // Copie une image complète (size doit égaler payloadBytes). Ne bloque jamais.
    bool Publish(const void* data, size_t size, int64_t qpc, int64_t pts100ns) {
        if (!hdr_ || size != hdr_->payloadBytes) return false;
        const uint64_t seq = hdr_->publishedSeq.load(std::memory_order_relaxed) + 1;
        const uint32_t idx = static_cast<uint32_t>(seq % kSlotCount);
        auto* s = Slot(idx);
        const uint64_t l = s->lock.load(std::memory_order_relaxed);
        s->lock.store(l + 1, std::memory_order_relaxed);          // impair : écriture
        std::atomic_thread_fence(std::memory_order_release);
        std::memcpy(Payload(s), data, size);
        s->frameSeq = seq; s->qpc = qpc; s->pts100ns = pts100ns;
        s->lock.store(l + 2, std::memory_order_release);          // pair : stable
        hdr_->latestSlot.store(idx, std::memory_order_release);
        hdr_->publishedSeq.store(seq, std::memory_order_release);
        return true;
    }

    void Close() { if (hdr_) hdr_->writerAlive.store(0, std::memory_order_release); }
    const RingHeader* Header() const { return hdr_; }

private:
    SlotHeader* Slot(uint32_t i) const {
        return reinterpret_cast<SlotHeader*>(base_ + hdr_->headerBytes + size_t(i) * hdr_->slotBytes);
    }
    static uint8_t* Payload(SlotHeader* s) { return reinterpret_cast<uint8_t*>(s) + sizeof(SlotHeader); }
    uint8_t* base_ = nullptr;
    RingHeader* hdr_ = nullptr;
};

enum class ReadResult { Ok, NoNewFrame, WriterGone, Invalid, Torn };

class Reader {
public:
    // Valide strictement l'en-tête avant tout accès (le segment n'est pas de confiance).
    bool Attach(const void* mem, size_t memBytes) {
        if (!mem || memBytes < sizeof(RingHeader)) return false;
        if ((reinterpret_cast<uintptr_t>(mem) & 63u) != 0) return false;
        auto* h = static_cast<const RingHeader*>(mem);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (h->magic != kRingMagic || h->abiVersion != kAbiVersion) return false;
        if (h->slotCount != kSlotCount || h->headerBytes != Align64(sizeof(RingHeader))) return false;
        if (h->format != uint32_t(PixelFormat::NV12) && h->format != uint32_t(PixelFormat::BGRA32)) return false;
        if (h->width == 0 || h->height == 0 || (h->width & 1) || (h->height & 1)) return false;
        if (h->payloadBytes == 0 || h->payloadBytes > kMaxPayload) return false;
        if (PayloadBytes(PixelFormat(h->format), h->width, h->height, h->strideY) != h->payloadBytes) return false;
        if (h->slotBytes != Align64(sizeof(SlotHeader) + h->payloadBytes)) return false;
        if (memBytes < size_t(h->headerBytes) + size_t(h->slotBytes) * kSlotCount) return false;
        base_ = static_cast<const uint8_t*>(mem);
        hdr_ = h;
        return true;
    }

    // Copie la dernière image si son numéro est > lastSeq. Jusqu'à 3 essais anti-déchirure.
    ReadResult TryRead(void* dst, size_t cap, FrameInfo& info, uint64_t lastSeq) const {
        if (!hdr_ || cap < hdr_->payloadBytes) return ReadResult::Invalid;
        for (int attempt = 0; attempt < 3; ++attempt) {
            const uint64_t pub = hdr_->publishedSeq.load(std::memory_order_acquire);
            if (pub == 0 || pub <= lastSeq)
                return hdr_->writerAlive.load(std::memory_order_acquire) ? ReadResult::NoNewFrame
                                                                         : ReadResult::WriterGone;
            const uint32_t idx = hdr_->latestSlot.load(std::memory_order_acquire) % kSlotCount;
            auto* s = reinterpret_cast<const SlotHeader*>(base_ + hdr_->headerBytes + size_t(idx) * hdr_->slotBytes);
            const uint64_t l1 = s->lock.load(std::memory_order_acquire);
            if (l1 & 1) continue;
            std::memcpy(dst, reinterpret_cast<const uint8_t*>(s) + sizeof(SlotHeader), hdr_->payloadBytes);
            const uint64_t seq = s->frameSeq; const int64_t q = s->qpc, p = s->pts100ns;
            std::atomic_thread_fence(std::memory_order_acquire);
            if (s->lock.load(std::memory_order_relaxed) != l1) continue;
            if (seq <= lastSeq) return ReadResult::NoNewFrame;
            info = {seq, q, p};
            return ReadResult::Ok;
        }
        return ReadResult::Torn;
    }
    const RingHeader* Header() const { return hdr_; }
private:
    const uint8_t* base_ = nullptr;
    const RingHeader* hdr_ = nullptr;
};

} // namespace vs
