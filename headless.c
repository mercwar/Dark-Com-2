// ============================================================================
// DarkComHeadless.c - Autonomous Headless WebView2 Engine DLL
// ============================================================================
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <webview2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPORT __declspec(dllexport)

typedef struct {
    char responseBuffer[4194304];
    volatile LONG isComplete;
    volatile LONG isNavComplete;
    volatile LONG isEnvComplete;
    volatile LONG isControllerComplete;
    ICoreWebView2Environment* webViewEnv;
    ICoreWebView2Controller* webViewCtrl;
    ICoreWebView2* webView;
    HWND hwndParent;
} HeadlessEngineCtx;

static HeadlessEngineCtx g_Eng = {0};

static HRESULT STDMETHODCALLTYPE Eng_QueryInterface(void* This, REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_ICoreWebView2WebMessageReceivedEventHandler) || 
        IsEqualIID(riid, &IID_ICoreWebView2NavigationCompletedEventHandler) ||
        IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) ||
        IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Eng_AddRef(void* This) { return 1; }
static ULONG STDMETHODCALLTYPE Eng_Release(void* This) { return 1; }

static HRESULT STDMETHODCALLTYPE Eng_EnvCompleted(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
    if (SUCCEEDED(result) && env) {
        g_Eng.webViewEnv = env;
        ICoreWebView2Environment_AddRef(env);
    }
    InterlockedExchange(&g_Eng.isEnvComplete, 1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Eng_CtrlCompleted(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* ctrl) {
    if (SUCCEEDED(result) && ctrl) {
        g_Eng.webViewCtrl = ctrl;
        ICoreWebView2Controller_AddRef(ctrl);
        ICoreWebView2Controller_get_CoreWebView2(ctrl, &g_Eng.webView);
    }
    InterlockedExchange(&g_Eng.isControllerComplete, 1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Eng_MsgReceived(ICoreWebView2WebMessageReceivedEventHandler* This, ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) {
    LPWSTR msgStr = NULL;
    if (SUCCEEDED(ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(args, &msgStr)) && msgStr) {
        WideCharToMultiByte(CP_UTF8, 0, msgStr, -1, g_Eng.responseBuffer, sizeof(g_Eng.responseBuffer) - 1, NULL, NULL);
        InterlockedExchange(&g_Eng.isComplete, 1);
        CoTaskMemFree(msgStr);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Eng_NavCompleted(ICoreWebView2NavigationCompletedEventHandler* This, ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) {
    InterlockedExchange(&g_Eng.isNavComplete, 1);
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = {
    (void*)Eng_QueryInterface, (void*)Eng_AddRef, (void*)Eng_Release, Eng_EnvCompleted
};
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctrlVtbl = {
    (void*)Eng_QueryInterface, (void*)Eng_AddRef, (void*)Eng_Release, Eng_CtrlCompleted
};

EXPORT void DarkCom_ShutdownEngine(void) {
    if (g_Eng.webViewCtrl) {
        ICoreWebView2Controller_Close(g_Eng.webViewCtrl);
        ICoreWebView2Controller_Release(g_Eng.webViewCtrl);
        g_Eng.webViewCtrl = NULL;
        g_Eng.webView = NULL;
    }
    if (g_Eng.webViewEnv) {
        ICoreWebView2Environment_Release(g_Eng.webViewEnv);
        g_Eng.webViewEnv = NULL;
    }
    if (g_Eng.hwndParent) {
        DestroyWindow(g_Eng.hwndParent);
        g_Eng.hwndParent = NULL;
    }
    CoUninitialize();
    memset(&g_Eng, 0, sizeof(g_Eng));
}

EXPORT int DarkCom_InitEngine(void) {
    if (g_Eng.webView) return 1;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 0;

    wchar_t cacheDir[MAX_PATH];
    GetTempPathW(MAX_PATH, cacheDir);
    wcscat_s(cacheDir, MAX_PATH, L"DarkCom2_Cache");

    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler envHandler = { &envVtbl };
    hr = CreateCoreWebView2EnvironmentWithOptions(NULL, cacheDir, NULL, &envHandler);
    if (FAILED(hr)) return 0;

    while (!InterlockedOr(&g_Eng.isEnvComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(5);
    }
    if (!g_Eng.webViewEnv) return 0;

    g_Eng.hwndParent = CreateWindowExW(0, L"STATIC", L"DarkCom2_HeadlessHost", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ctrlHandler = { &ctrlVtbl };
    hr = ICoreWebView2Environment_CreateCoreWebView2Controller(g_Eng.webViewEnv, g_Eng.hwndParent, &ctrlHandler);
    if (FAILED(hr)) return 0;

    while (!InterlockedOr(&g_Eng.isControllerComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(5);
    }

    return (g_Eng.webView != NULL);
}

EXPORT int DarkCom_ExecuteFetch(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
    if (!g_Eng.webView) {
        if (!DarkCom_InitEngine()) return 0;
    }

    wchar_t wUrl[2048];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wUrl, 2048);

    wchar_t wOrigin[1024] = L"http://localhost";
    if (wcsncmp(wUrl, L"http://", 7) == 0 || wcsncmp(wUrl, L"https://", 8) == 0) {
        const wchar_t* pathStart = wcschr(wUrl + 8, L'/');
        if (pathStart) {
            size_t len = (size_t)(pathStart - wUrl);
            wcsncpy_s(wOrigin, 1024, wUrl, len);
        } else {
            wcscpy_s(wOrigin, 1024, wUrl);
        }
    }

    InterlockedExchange(&g_Eng.isNavComplete, 0);
    ICoreWebView2NavigationCompletedEventHandlerVtbl navVtbl = { (void*)Eng_QueryInterface, (void*)Eng_AddRef, (void*)Eng_Release, Eng_NavCompleted };
    ICoreWebView2NavigationCompletedEventHandler navHandler = { &navVtbl };
    EventRegistrationToken navToken;
    ICoreWebView2_add_NavigationCompleted(g_Eng.webView, &navHandler, &navToken);

    ICoreWebView2_Navigate(g_Eng.webView, wOrigin);
    while (!InterlockedOr(&g_Eng.isNavComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(5);
    }
    ICoreWebView2_remove_NavigationCompleted(g_Eng.webView, navToken);

    InterlockedExchange(&g_Eng.isComplete, 0);
    g_Eng.responseBuffer[0] = '\0';

    ICoreWebView2WebMessageReceivedEventHandlerVtbl msgVtbl = { (void*)Eng_QueryInterface, (void*)Eng_AddRef, (void*)Eng_Release, Eng_MsgReceived };
    ICoreWebView2WebMessageReceivedEventHandler msgHandler = { &msgVtbl };
    EventRegistrationToken msgToken;
    ICoreWebView2_add_WebMessageReceived(g_Eng.webView, &msgHandler, &msgToken);

    wchar_t jsScript[16384];
    // FIX: Only inject body if postData is present AND method is NOT GET or HEAD
    int hasBody = (postData && _stricmp(method, "GET") != 0 && _stricmp(method, "HEAD") != 0);

    if (hasBody) {
        wchar_t wPostData[4096];
        MultiByteToWideChar(CP_UTF8, 0, postData, -1, wPostData, 4096);
        swprintf_s(jsScript, 16384,
            L"(async function() {"
            L"  try {"
            L"    let res = await fetch('%ls', { method: '%S', headers: {'Content-Type': 'application/json'}, body: '%ls' });"
            L"    let buf = await res.arrayBuffer();"
            L"    let bytes = new Uint8Array(buf), binary = '';"
            L"    for (let i=0; i<bytes.length; i+=0x8000) binary += String.fromCharCode.apply(null, bytes.subarray(i, i+0x8000));"
            L"    window.chrome.webview.postMessage(btoa(binary));"
            L"  } catch(e) { window.chrome.webview.postMessage('ERROR:' + e.toString()); }"
            L"})();",
            wUrl, method, wPostData
        );
    } else {
        swprintf_s(jsScript, 16384,
            L"(async function() {"
            L"  try {"
            L"    let res = await fetch('%ls', { method: '%S' });"
            L"    let buf = await res.arrayBuffer();"
            L"    let bytes = new Uint8Array(buf), binary = '';"
            L"    for (let i=0; i<bytes.length; i+=0x8000) binary += String.fromCharCode.apply(null, bytes.subarray(i, i+0x8000));"
            L"    window.chrome.webview.postMessage(btoa(binary));"
            L"  } catch(e) { window.chrome.webview.postMessage('ERROR:' + e.toString()); }"
            L"})();",
            wUrl, method
        );
    }

    ICoreWebView2ExecuteScriptCompletedHandlerVtbl execVtbl = { (void*)Eng_QueryInterface, (void*)Eng_AddRef, (void*)Eng_Release, (void*)Eng_AddRef };
    ICoreWebView2ExecuteScriptCompletedHandler execHandler = { &execVtbl };
    ICoreWebView2_ExecuteScript(g_Eng.webView, jsScript, &execHandler);

    while (!InterlockedOr(&g_Eng.isComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(5);
    }

    ICoreWebView2_remove_WebMessageReceived(g_Eng.webView, msgToken);

    strncpy_s(outBuffer, outBufferSize, g_Eng.responseBuffer, _TRUNCATE);
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_DETACH) {
        // Safe detachment check
    }
    return TRUE;
}

EXPORT int RunHeadlessInit(void) {
    return DarkCom_InitEngine();
}

EXPORT int RunHeadlessRequest(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
    return DarkCom_ExecuteFetch(url, method, postData, outBuffer, outBufferSize);
}

EXPORT void RunHeadlessShutdown(void) {
    DarkCom_ShutdownEngine();
}