// ============================================================================
// DarkComHeadless.c - Autonomous Headless WebView2 Engine with Fixed Binary Handling
// ============================================================================
#define BUILDING_DARKCOM_DLL
#include "headless.h"

#define COBJMACROS
#include <windows.h>
#include <stdbool.h>
#include <ole2.h>
#include <webview2.h>
#include <winhttp.h>
#include <wininet.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_BUFFER_DUMP
} LogLevel;

typedef void (CALLBACK *NetworkLogCallback)(LogLevel level, const char *message);

typedef struct {
    bool enable_logging;
    NetworkLogCallback log_callback;
    size_t initial_buffer_size;
    size_t max_buffer_size;
    char custom_user_agent[256];
} NetworkConfig;

typedef BOOL (WINAPI *pfnDc2ws2Init)(void);
typedef void (WINAPI *pfnDc2ws2Cleanup)(void);
typedef HANDLE (WINAPI *pfnDc2ws2DownloadAsync)(const char *url, const char *dest_path, const NetworkConfig *config);

typedef struct {
    char* responseBuffer;
    DWORD responseBufferSize;
    DWORD actualBytesWritten;
    volatile LONG isComplete;
    volatile LONG isNavComplete;
    volatile LONG isEnvComplete;
    volatile LONG isControllerComplete;
    ICoreWebView2Environment* webViewEnv;
    ICoreWebView2Controller* webViewCtrl;
    ICoreWebView2* webView;
    HWND hwndParent;
    wchar_t currentOrigin[1024];
    EventRegistrationToken msgToken;
} HeadlessEngineCtx;

static HeadlessEngineCtx g_Eng = {0};

typedef struct { ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl* lpVtbl; LONG refCount; } EnvCompletedHandler;
typedef struct { ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl; LONG refCount; } CtrlCompletedHandler;
typedef struct { ICoreWebView2WebMessageReceivedEventHandlerVtbl* lpVtbl; LONG refCount; } MsgReceivedHandler;
typedef struct { ICoreWebView2NavigationCompletedEventHandlerVtbl* lpVtbl; LONG refCount; } NavCompletedHandler;
typedef struct { ICoreWebView2ExecuteScriptCompletedHandlerVtbl* lpVtbl; LONG refCount; } ExecCompletedHandler;

static HRESULT STDMETHODCALLTYPE Generic_QueryInterface(void* This, REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = This;
    return S_OK;
}
static ULONG STDMETHODCALLTYPE Generic_AddRef(void* This) {
    return InterlockedIncrement(&((EnvCompletedHandler*)This)->refCount);
}
static ULONG STDMETHODCALLTYPE Generic_Release(void* This) {
    LONG val = InterlockedDecrement(&((EnvCompletedHandler*)This)->refCount);
    if (val == 0) free(This);
    return val;
}

static HRESULT STDMETHODCALLTYPE Env_Completed(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
    if (SUCCEEDED(result) && env) {
        g_Eng.webViewEnv = env;
        ICoreWebView2Environment_AddRef(env);
    }
    InterlockedExchange(&g_Eng.isEnvComplete, 1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Ctrl_Completed(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* ctrl) {
    if (SUCCEEDED(result) && ctrl) {
        g_Eng.webViewCtrl = ctrl;
        ICoreWebView2Controller_AddRef(ctrl);
        ICoreWebView2Controller_get_CoreWebView2(ctrl, &g_Eng.webView);
    }
    InterlockedExchange(&g_Eng.isControllerComplete, 1);
    return S_OK;
}

// Robust Base64 decoder that writes exact binary bytes and tracks actual length
static DWORD SafeBase64Decode(const wchar_t* wSrc, char* dst, DWORD dstLen) {
    DWORD wLen = (DWORD)wcslen(wSrc);
    if (wLen == 0) return 0;

    // Convert wide string to ANSI base64 string directly
    char* b64 = (char*)malloc(wLen + 1);
    if (!b64) return 0;
    WideCharToMultiByte(CP_ACP, 0, wSrc, -1, b64, wLen + 1, NULL, NULL);

    DWORD decodedSize = dstLen;
    if (!CryptStringToBinaryA(b64, 0, CRYPT_STRING_BASE64, (BYTE*)dst, &decodedSize, NULL, NULL)) {
        decodedSize = 0;
    }

    free(b64);
    return decodedSize;
}

static HRESULT STDMETHODCALLTYPE Msg_Received(ICoreWebView2WebMessageReceivedEventHandler* This, ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) {
    LPWSTR msgStr = NULL;
    if (SUCCEEDED(ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(args, &msgStr)) && msgStr) {
        if (g_Eng.responseBuffer && g_Eng.responseBufferSize > 0) {
            if (wcsncmp(msgStr, L"ERROR:", 6) == 0) {
                WideCharToMultiByte(CP_UTF8, 0, msgStr, -1, g_Eng.responseBuffer, g_Eng.responseBufferSize, NULL, NULL);
                g_Eng.actualBytesWritten = (DWORD)strlen(g_Eng.responseBuffer);
            } else {
                // Decode base64 payload precisely into raw binary bytes
                g_Eng.actualBytesWritten = SafeBase64Decode(msgStr, g_Eng.responseBuffer, g_Eng.responseBufferSize);
            }
        }
        InterlockedExchange(&g_Eng.isComplete, 1);
        CoTaskMemFree(msgStr);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Nav_Completed(ICoreWebView2NavigationCompletedEventHandler* This, ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) {
    InterlockedExchange(&g_Eng.isNavComplete, 1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Exec_Completed(ICoreWebView2ExecuteScriptCompletedHandler* This, HRESULT result, LPCWSTR resultObjectAsJson) {
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Env_Completed };
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctrlVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Ctrl_Completed };
static ICoreWebView2WebMessageReceivedEventHandlerVtbl msgVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Msg_Received };
static ICoreWebView2NavigationCompletedEventHandlerVtbl navVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Nav_Completed };
static ICoreWebView2ExecuteScriptCompletedHandlerVtbl execVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Exec_Completed };

DARKCOM_API void DarkCom_ShutdownEngine(void) {
    if (g_Eng.webView) {
        ICoreWebView2_remove_WebMessageReceived(g_Eng.webView, g_Eng.msgToken);
    }
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

DARKCOM_API int DarkCom_InitEngine(void) {
    if (g_Eng.webView) return 1;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return 0;

    wchar_t cacheDir[MAX_PATH];
    GetTempPathW(MAX_PATH, cacheDir);
    wcscat_s(cacheDir, MAX_PATH, L"DarkCom2_Cache");

    EnvCompletedHandler* envHandler = (EnvCompletedHandler*)malloc(sizeof(EnvCompletedHandler));
    if (!envHandler) return 0;
    envHandler->lpVtbl = &envVtbl;
    envHandler->refCount = 1;

    hr = CreateCoreWebView2EnvironmentWithOptions(NULL, cacheDir, NULL, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)envHandler);
    if (FAILED(hr)) { free(envHandler); return 0; }

    while (!InterlockedOr(&g_Eng.isEnvComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(0);
    }
    if (!g_Eng.webViewEnv) return 0;

    g_Eng.hwndParent = CreateWindowExW(0, L"STATIC", L"DarkCom2_HeadlessHost", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

    CtrlCompletedHandler* ctrlHandler = (CtrlCompletedHandler*)malloc(sizeof(CtrlCompletedHandler));
    if (!ctrlHandler) return 0;
    ctrlHandler->lpVtbl = &ctrlVtbl;
    ctrlHandler->refCount = 1;

    hr = ICoreWebView2Environment_CreateCoreWebView2Controller(g_Eng.webViewEnv, g_Eng.hwndParent, (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)ctrlHandler);
    if (FAILED(hr)) { free(ctrlHandler); return 0; }

    while (!InterlockedOr(&g_Eng.isControllerComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        Sleep(0);
    }

    if (g_Eng.webView) {
        MsgReceivedHandler* msgHandler = (MsgReceivedHandler*)malloc(sizeof(MsgReceivedHandler));
        if (msgHandler) {
            msgHandler->lpVtbl = &msgVtbl;
            msgHandler->refCount = 1;
            ICoreWebView2_add_WebMessageReceived(g_Eng.webView, (ICoreWebView2WebMessageReceivedEventHandler*)msgHandler, &g_Eng.msgToken);
        }
    }

    g_Eng.currentOrigin[0] = L'\0';
    return (g_Eng.webView != NULL);
}

DARKCOM_API int DarkCom_ExecuteFetch(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
    if (!g_Eng.webView) {
        if (!DarkCom_InitEngine()) return -1;
    }

    g_Eng.responseBuffer = outBuffer;
    g_Eng.responseBufferSize = outBufferSize;
    g_Eng.actualBytesWritten = 0;
    if (outBufferSize > 0) outBuffer[0] = '\0';

    wchar_t wUrl[2048];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wUrl, 2048);

    URL_COMPONENTSW urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {0};
    wchar_t schemeName[32] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszScheme = schemeName;
    urlComp.dwSchemeLength = 32;

    wchar_t targetOrigin[512] = L"about:blank";
    if (WinHttpCrackUrl(wUrl, (DWORD)wcslen(wUrl), 0, &urlComp)) {
        swprintf_s(targetOrigin, 512, L"%ls://%ls", urlComp.lpszScheme, urlComp.lpszHostName);
    }

    LPWSTR currentSource = NULL;
    ICoreWebView2_get_Source(g_Eng.webView, &currentSource);

    if (!currentSource || wcsstr(currentSource, targetOrigin) == NULL) {
        InterlockedExchange(&g_Eng.isNavComplete, 0);
        NavCompletedHandler* navHandler = (NavCompletedHandler*)malloc(sizeof(NavCompletedHandler));
        if (navHandler) {
            navHandler->lpVtbl = &navVtbl;
            navHandler->refCount = 1;
            EventRegistrationToken navToken;
            ICoreWebView2_add_NavigationCompleted(g_Eng.webView, (ICoreWebView2NavigationCompletedEventHandler*)navHandler, &navToken);

            ICoreWebView2_Navigate(g_Eng.webView, targetOrigin);

            DWORD navTick = GetTickCount();
            while (!InterlockedOr(&g_Eng.isNavComplete, 0)) {
                MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
                Sleep(0);
                if (GetTickCount() - navTick > 8000) break;
            }
            ICoreWebView2_remove_NavigationCompleted(g_Eng.webView, navToken);
        }
    }
    if (currentSource) CoTaskMemFree(currentSource);

    InterlockedExchange(&g_Eng.isComplete, 0);

    wchar_t* jsScript = (wchar_t*)malloc(32768 * sizeof(wchar_t));
    if (!jsScript) return -1;

    int hasBody = (postData && _stricmp(method, "GET") != 0 && _stricmp(method, "HEAD") != 0);

    // Fixed JS snippet using standard safe chunked base64 conversion for binary arrays
    const wchar_t* fetchTmpl = 
        L"(async function() {"
        L"  try {"
        L"    let res = await fetch('%ls', { method: '%S'%ls%ls, redirect: 'follow' });"
        L"    if (!res.ok) { window.chrome.webview.postMessage('ERROR: HTTP status ' + res.status); return; }"
        L"    let buf = await res.arrayBuffer();"
        L"    let bytes = new Uint8Array(buf);"
        L"    let CHUNK = 0x8000;"
        L"    let binary = '';"
        L"    for (let i = 0; i < bytes.length; i += CHUNK) {"
        L"      let chunk = bytes.subarray(i, Math.min(i + CHUNK, bytes.length));"
        L"      binary += String.fromCharCode.apply(null, chunk);"
        L"    }"
        L"    window.chrome.webview.postMessage(btoa(binary));"
        L"  } catch(e) { window.chrome.webview.postMessage('ERROR:' + e.toString()); }"
        L"})();";

    wchar_t wPostData[8192] = {0};
    wchar_t optHeaders[128] = L"";
    wchar_t optBody[8192] = L"";

    if (hasBody && postData) {
        MultiByteToWideChar(CP_UTF8, 0, postData, -1, wPostData, 8192);
        wcscpy_s(optHeaders, 128, L", headers: {'Content-Type': 'application/json'}");
        swprintf_s(optBody, 8192, L", body: '%ls'", wPostData);
    }

    swprintf_s(jsScript, 32768, fetchTmpl, wUrl, method, optHeaders, optBody);

    ExecCompletedHandler* execHandler = (ExecCompletedHandler*)malloc(sizeof(ExecCompletedHandler));
    if (execHandler) {
        execHandler->lpVtbl = &execVtbl;
        execHandler->refCount = 1;
        ICoreWebView2_ExecuteScript(g_Eng.webView, jsScript, (ICoreWebView2ExecuteScriptCompletedHandler*)execHandler);
    }
    free(jsScript);

    DWORD tickStart = GetTickCount();
    while (!InterlockedOr(&g_Eng.isComplete, 0)) {
        MSG msg; while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        (void)msg;
        Sleep(0);
        if (GetTickCount() - tickStart > 15000) break;
    }

    // Return the exact binary byte count instead of relying on strlen() which cuts off at null bytes
    return (int)g_Eng.actualBytesWritten;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_DETACH) {
        DarkCom_ShutdownEngine();
    }
    return TRUE;
}

DARKCOM_API int RunHeadlessInit(void) { 
    return DarkCom_InitEngine(); 
}

DARKCOM_API int RunHeadlessRequest(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
    return DarkCom_ExecuteFetch(url, method, postData, outBuffer, outBufferSize);
}

DARKCOM_API void RunHeadlessShutdown(void) { 
    DarkCom_ShutdownEngine(); 
}