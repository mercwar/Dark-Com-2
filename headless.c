// ============================================================================
// DarkComHeadless.c - Autonomous Headless WebView2 Engine with Dynamic dc2ws2.dll Loading
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

// Re-define necessary types from dc2ws2.h locally for dynamic loading
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

// Function pointer types for dynamic loading of dc2ws2.dll
typedef BOOL (WINAPI *pfnDc2ws2Init)(void);
typedef void (WINAPI *pfnDc2ws2Cleanup)(void);
typedef HANDLE (WINAPI *pfnDc2ws2DownloadAsync)(const char *url, const char *dest_path, const NetworkConfig *config);

typedef struct {
    char* responseBuffer;
    DWORD responseBufferSize;
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

// Forward declarations for COM Handlers
typedef struct {
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
} EnvCompletedHandler;

typedef struct {
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
} CtrlCompletedHandler;

typedef struct {
    ICoreWebView2WebMessageReceivedEventHandlerVtbl* lpVtbl;
    LONG refCount;
} MsgReceivedHandler;

typedef struct {
    ICoreWebView2NavigationCompletedEventHandlerVtbl* lpVtbl;
    LONG refCount;
} NavCompletedHandler;

typedef struct {
    ICoreWebView2ExecuteScriptCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
} ExecCompletedHandler;

// --- COM Handler Implementations ---
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

// Base64 helper to decode WebView2 binary payloads back to raw bytes
static int Base64Decode(const wchar_t* src, char* dst, DWORD dstLen) {
    DWORD len = (DWORD)wcslen(src);
    if (len == 0) return 0;

    char* b64 = (char*)malloc(len + 1);
    if (!b64) return 0;
    WideCharToMultiByte(CP_ACP, 0, src, -1, b64, len + 1, NULL, NULL);

    DWORD decodedSize = dstLen;
    if (!CryptStringToBinaryA(b64, 0, CRYPT_STRING_BASE64, (BYTE*)dst, &decodedSize, NULL, NULL)) {
        strncpy_s(dst, dstLen, b64, _TRUNCATE);
        decodedSize = (DWORD)strlen(dst);
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
            } else {
                Base64Decode(msgStr, g_Eng.responseBuffer, g_Eng.responseBufferSize);
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

// Vtables
static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Env_Completed };
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctrlVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Ctrl_Completed };
static ICoreWebView2WebMessageReceivedEventHandlerVtbl msgVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Msg_Received };
static ICoreWebView2NavigationCompletedEventHandlerVtbl navVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Nav_Completed };
static ICoreWebView2ExecuteScriptCompletedHandlerVtbl execVtbl = { (void*)Generic_QueryInterface, (void*)Generic_AddRef, (void*)Generic_Release, Exec_Completed };

// --- Dynamic dc2ws2.dll Integration Fast-Path Hook (Aligned Clean Body Extraction) ---
static int FastDc2Ws2Fetch(const char* url, char* outBuffer, DWORD outBufferSize) {
    if (!url || !outBuffer || outBufferSize == 0) return -1;

    HMODULE hDc2Ws2 = LoadLibraryA("dc2ws2.dll");
    if (!hDc2Ws2) {
        return -1;
    }

    pfnDc2ws2Init dc2Init = (pfnDc2ws2Init)GetProcAddress(hDc2Ws2, "Dc2ws2Init");
    pfnDc2ws2Cleanup dc2Cleanup = (pfnDc2ws2Cleanup)GetProcAddress(hDc2Ws2, "Dc2ws2Cleanup");
    pfnDc2ws2DownloadAsync dc2DownloadAsync = (pfnDc2ws2DownloadAsync)GetProcAddress(hDc2Ws2, "Dc2ws2DownloadAsync");

    if (!dc2Init || !dc2Cleanup || !dc2DownloadAsync) {
        FreeLibrary(hDc2Ws2);
        return -1;
    }

    char currentUrl[2048];
    strncpy_s(currentUrl, sizeof(currentUrl), url, _TRUNCATE);
    
    char* flagPos = strstr(currentUrl, "-nojs");
    if (flagPos) {
        *flagPos = '\0';
        while (flagPos > currentUrl && (*(flagPos - 1) == ' ' || *(flagPos - 1) == '\t')) {
            *(--flagPos) = '\0';
        }
    }

    if (!dc2Init()) {
        FreeLibrary(hDc2Ws2);
        return -1;
    }

    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    strcat_s(tempPath, MAX_PATH, "dc2_temp_payload.bin");

    NetworkConfig cfg = {0};
    cfg.enable_logging = true;
    cfg.initial_buffer_size = 65536;
    GetEnvironmentVariableA("DC2_USER_AGENT", cfg.custom_user_agent, sizeof(cfg.custom_user_agent));

    HANDLE hDownload = dc2DownloadAsync(currentUrl, tempPath, &cfg);
    if (!hDownload) {
        dc2Cleanup();
        FreeLibrary(hDc2Ws2);
        return -1;
    }

    WaitForSingleObject(hDownload, INFINITE);
    CloseHandle(hDownload);

    FILE* f = NULL;
    int bytesReadCount = -1;
    
    for (int retry = 0; retry < 5; retry++) {
        if (fopen_s(&f, tempPath, "rb") == 0 && f) {
            break;
        }
        Sleep(50);
    }

    if (f) {
        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (fileSize > 0) {
            char* fileBuffer = (char*)malloc(fileSize + 1);
            if (fileBuffer) {
                long totalRead = (long)fread(fileBuffer, 1, fileSize, f);
                fileBuffer[totalRead] = '\0';

                // Locate exact HTTP header/body boundary (\r\n\r\n)
                char* body = NULL;
                for (long i = 0; i + 3 < totalRead; i++) {
                    if (fileBuffer[i] == '\r' && fileBuffer[i+1] == '\n' &&
                        fileBuffer[i+2] == '\r' && fileBuffer[i+3] == '\n') {
                        body = fileBuffer + i + 4;
                        break;
                    }
                }

                if (body) {
                    long bodyLen = (long)(totalRead - (body - fileBuffer));
                    
                    // Check Content-Length if present to ensure exact byte alignment
                    long content_length = -1;
                    for (char* p = fileBuffer; p < body; p++) {
                        if (_strnicmp(p, "Content-Length:", 15) == 0) {
                            content_length = atol(p + 15);
                            break;
                        }
                    }

                    if (content_length > 0 && content_length < bodyLen) {
                        bodyLen = content_length;
                    }

                    if (bodyLen > 0 && (DWORD)bodyLen < outBufferSize) {
                        memcpy(outBuffer, body, bodyLen);
                        outBuffer[bodyLen] = '\0';
                        bytesReadCount = (int)bodyLen;
                    }
                } else {
                    if ((DWORD)totalRead < outBufferSize) {
                        memcpy(outBuffer, fileBuffer, totalRead);
                        outBuffer[totalRead] = '\0';
                        bytesReadCount = (int)totalRead;
                    }
                }
                free(fileBuffer);
            }
        }
        fclose(f);
        remove(tempPath);
    }

    dc2Cleanup();
    FreeLibrary(hDc2Ws2);
    return bytesReadCount;
}

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
    int isNoJs = (url && strstr(url, "-nojs") != NULL) || (postData && strstr(postData, "-nojs") != NULL);
    if (isNoJs) {
        return FastDc2Ws2Fetch(url, outBuffer, outBufferSize);
    }

    if (!g_Eng.webView) {
        if (!DarkCom_InitEngine()) return -1;
    }

    g_Eng.responseBuffer = outBuffer;
    g_Eng.responseBufferSize = outBufferSize;
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

    const wchar_t* fetchTmpl = 
        L"(async function() {"
        L"  try {"
        L"    let res = await fetch('%ls', { method: '%S'%ls%ls, redirect: 'follow' });"
        L"    if (!res.ok) { window.chrome.webview.postMessage('ERROR: HTTP status ' + res.status); return; }"
        L"    let buf = await res.arrayBuffer();"
        L"    let bytes = new Uint8Array(buf);"
        L"    let len = bytes.byteLength;"
        L"    let CHUNK = 0x8000;"
        L"    let binary = '';"
        L"    for (let i = 0; i < len; i += CHUNK) {"
        L"      let chunk = bytes.subarray(i, Math.min(i + CHUNK, len));"
        L"      let arr = [];"
        L"      for (let j = 0; j < chunk.length; j++) { arr.push(chunk[j]); }"
        L"      binary += String.fromCharCode.apply(null, arr);"
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

    return (int)strlen(outBuffer);
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