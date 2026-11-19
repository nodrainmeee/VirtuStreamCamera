#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <audioclient.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <wininet.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <wrl/client.h>
#include <string>
#include <algorithm>
#include <iterator>
#include <new>
#include <memory>
#include <atomic>

using Microsoft::WRL::ComPtr;

namespace {
constexpr wchar_t kClassName[] = L"VirtuStreamCameraWindow";
constexpr UINT WM_MEDIA_EVENT = WM_APP + 10;
constexpr UINT WM_MEDIA_REFRESH = WM_APP + 11;
enum : int { IDC_OPEN = 1001, IDC_PLAY = 1002, IDC_LOOP = 1003, IDC_STATUS = 1004, IDC_QUALITY = 1005 };

HWND g_main = nullptr;
HWND g_preview = nullptr;
HWND g_status = nullptr;
HWND g_play = nullptr;
HWND g_loop = nullptr;
HWND g_quality = nullptr;
ComPtr<IMFMediaEngine> g_engine;
ComPtr<IMFMediaEngineNotify> g_notify;
bool g_isPlaying = false;
bool g_looping = true;
std::wstring g_currentFile;

std::wstring HrText(HRESULT hr) {
    wchar_t buf[32]{};
    swprintf_s(buf, L"0x%08X", static_cast<unsigned int>(hr));
    return buf;
}

class MediaNotify final : public IMFMediaEngineNotify {
public:
    explicit MediaNotify(HWND hwnd) : hwnd_(hwnd) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMFMediaEngineNotify)) {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = --refs_;
        if (!n) delete this;
        return n;
    }
    STDMETHODIMP EventNotify(DWORD event, DWORD_PTR, DWORD) override {
        if (hwnd_ && IsWindow(hwnd_)) PostMessageW(hwnd_, WM_MEDIA_EVENT, event, 0);
        return S_OK;
    }
private:
    std::atomic<ULONG> refs_{1};
    HWND hwnd_{};
};

void SetStatus(const std::wstring& text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

HRESULT CreateEngine() {
    if (g_engine) return S_OK;

    ComPtr<IMFAttributes> attrs;
    HRESULT hr = MFCreateAttributes(&attrs, 4);
    if (FAILED(hr)) return hr;

    auto* callback = new (std::nothrow) MediaNotify(g_main);
    if (!callback) return E_OUTOFMEMORY;
    g_notify.Attach(callback);

    hr = attrs->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, g_notify.Get());
    if (FAILED(hr)) return hr;

    // Le moteur rend l'aperçu dans une fenêtre enfant dédiée.
    hr = attrs->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND,
                          static_cast<UINT64>(reinterpret_cast<ULONG_PTR>(g_preview)));
    if (FAILED(hr)) return hr;

    // L'application de contrôle ne fait pas de monitoring audio pour économiser les ressources.
    hr = attrs->SetUINT32(MF_MEDIA_ENGINE_AUDIO_CATEGORY, AudioCategory_Media);
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaEngineClassFactory> factory;
    hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return hr;

    hr = factory->CreateInstance(0, attrs.Get(), &g_engine);
    if (FAILED(hr)) return hr;

    g_engine->SetMuted(TRUE);
    g_engine->SetLoop(g_looping ? TRUE : FALSE);
    g_engine->SetPreload(MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
    return S_OK;
}

HRESULT ToFileUri(const std::wstring& path, std::wstring& uri) {
    wchar_t buffer[4096]{};
    DWORD chars = static_cast<DWORD>(std::size(buffer));
    HRESULT hr = UrlCreateFromPathW(path.c_str(), buffer, &chars, 0);
    if (FAILED(hr)) return hr;
    uri.assign(buffer);
    return S_OK;
}

HRESULT OpenMedia(const std::wstring& path) {
    HRESULT hr = CreateEngine();
    if (FAILED(hr)) return hr;

    std::wstring uri;
    hr = ToFileUri(path, uri);
    if (FAILED(hr)) return hr;

    BSTR source = SysAllocString(uri.c_str());
    if (!source) return E_OUTOFMEMORY;
    hr = g_engine->SetSource(source);
    SysFreeString(source);
    if (FAILED(hr)) return hr;

    g_currentFile = path;
    g_isPlaying = false;
    SetWindowTextW(g_play, L"Lire");
    SetStatus(L"Chargement de la vidéo… La sortie caméra Windows sera ajoutée au moteur natif.");
    return S_OK;
}

std::wstring PickVideo(HWND owner) {
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dialog)))) return {};
    COMDLG_FILTERSPEC filters[] = {
        { L"Fichiers vidéo", L"*.mp4;*.mov;*.m4v;*.wmv;*.avi;*.mkv;*.webm" },
        { L"Tous les fichiers", L"*.*" }
    };
    dialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters);
    dialog->SetFileTypeIndex(1);
    dialog->SetTitle(L"Sélectionner une vidéo locale");
    if (FAILED(dialog->Show(owner))) return {};
    ComPtr<IShellItem> item;
    if (FAILED(dialog->GetResult(&item))) return {};
    PWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw)) || !raw) return {};
    std::wstring result(raw);
    CoTaskMemFree(raw);
    return result;
}

void Layout(HWND hwnd) {
    RECT r{};
    GetClientRect(hwnd, &r);
    const int width = r.right - r.left;
    const int height = r.bottom - r.top;
    const int margin = 18;
    const int barH = 52;
    const int statusH = 54;
    const int previewTop = 76;
    const int previewW = width - margin * 2;
    const int previewH = height - previewTop - statusH - 30;
    if (g_preview) MoveWindow(g_preview, margin, previewTop, std::max(320, previewW), std::max(180, previewH), TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_OPEN), margin, 16, 150, 34, TRUE);
    MoveWindow(g_play, margin + 160, 16, 90, 34, TRUE);
    MoveWindow(g_loop, margin + 260, 16, 120, 34, TRUE);
    MoveWindow(g_quality, margin + 400, 16, 190, 34, TRUE);
    MoveWindow(g_status, margin, height - statusH + 2, width - margin * 2, statusH - 8, TRUE);
    (void)barH;
}

void TogglePlay() {
    if (!g_engine) return;
    if (g_isPlaying) {
        g_engine->Pause();
        g_isPlaying = false;
        SetWindowTextW(g_play, L"Lire");
        SetStatus(L"Lecture en pause. La prévisualisation reste disponible.");
    } else {
        HRESULT hr = g_engine->Play();
        if (SUCCEEDED(hr)) {
            g_isPlaying = true;
            SetWindowTextW(g_play, L"Pause");
            SetStatus(L"Lecture active. La caméra virtuelle n'est pas encore installée dans cette base initiale.");
        } else SetStatus(L"Impossible de lire la vidéo : " + HrText(hr));
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_main = hwnd;
        g_preview = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BLACKRECT,
                                    18, 76, 900, 480, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Choisir une vidéo", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        18, 16, 150, 34, hwnd, reinterpret_cast<HMENU>(IDC_OPEN), nullptr, nullptr);
        g_play = CreateWindowExW(0, L"BUTTON", L"Lire", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 178, 16, 90, 34, hwnd, reinterpret_cast<HMENU>(IDC_PLAY), nullptr, nullptr);
        g_loop = CreateWindowExW(0, L"BUTTON", L"Boucle : activée", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                 278, 16, 120, 34, hwnd, reinterpret_cast<HMENU>(IDC_LOOP), nullptr, nullptr);
        SendMessageW(g_loop, BM_SETCHECK, BST_CHECKED, 0);
        g_quality = CreateWindowExW(0, L"STATIC", L"Sortie cible : 1280 × 720 · 30 FPS", WS_CHILD | WS_VISIBLE,
                                    418, 16, 260, 34, hwnd, reinterpret_cast<HMENU>(IDC_QUALITY), nullptr, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"Prêt. Choisissez une vidéo pour commencer.", WS_CHILD | WS_VISIBLE,
                                   18, 570, 900, 46, hwnd, reinterpret_cast<HMENU>(IDC_STATUS), nullptr, nullptr);
        Layout(hwnd);
        return 0;
    }
    case WM_SIZE:
        Layout(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_OPEN: {
            auto path = PickVideo(hwnd);
            if (!path.empty()) {
                HRESULT hr = OpenMedia(path);
                if (FAILED(hr)) SetStatus(L"Erreur d'ouverture : " + HrText(hr));
            }
            return 0;
        }
        case IDC_PLAY:
            TogglePlay();
            return 0;
        case IDC_LOOP:
            g_looping = (SendMessageW(g_loop, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (g_engine) g_engine->SetLoop(g_looping ? TRUE : FALSE);
            SetWindowTextW(g_loop, g_looping ? L"Boucle : activée" : L"Boucle : arrêtée");
            return 0;
        }
        break;
    case WM_MEDIA_EVENT:
        if (wp == MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA) {
            DWORD videoW = 0;
            DWORD videoH = 0;
            if (g_engine) {
                // IMFMediaEngine n'a pas GetVideoWidth/GetVideoHeight : on utilise GetNativeVideoSize.
                if (FAILED(g_engine->GetNativeVideoSize(&videoW, &videoH))) {
                    videoW = 0;
                    videoH = 0;
                }
            }
            UINT32 w = static_cast<UINT32>(videoW);
            UINT32 h = static_cast<UINT32>(videoH);
            wchar_t line[256]{};
            swprintf_s(line, L"Vidéo chargée : %u × %u. Prévisualisation locale uniquement ; caméra Windows en cours de développement.", w, h);
            SetStatus(line);
        } else if (wp == MF_MEDIA_ENGINE_EVENT_ERROR) {
            SetStatus(L"Erreur Media Foundation pendant la lecture. Vérifiez le format et les codecs du fichier.");
        } else if (wp == MF_MEDIA_ENGINE_EVENT_ENDED) {
            if (!g_looping) {
                g_isPlaying = false;
                SetWindowTextW(g_play, L"Lire");
            }
        }
        return 0;
    case WM_DESTROY:
        if (g_engine) {
            g_engine->Pause();
            g_engine.Reset();
        }
        g_notify.Reset();
        MFShutdown();
        CoUninitialize();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { CoUninitialize(); return 2; }

    INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&controls);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(32, 33, 36));
    wc.lpszClassName = kClassName;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassExW(&wc)) { MFShutdown(); CoUninitialize(); return 3; }

    HWND hwnd = CreateWindowExW(0, kClassName, L"VirtuStream Camera — Préversion technique",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) { MFShutdown(); CoUninitialize(); return 4; }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    DeleteObject(wc.hbrBackground);
    return static_cast<int>(message.wParam);
}
