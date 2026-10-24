/* ============================================================================
   headless.c - WebView2 Headless Engine DLL (Pure C) - Direct Origin POST Edition
   ============================================================================ */
#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <oleacc.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <shlwapi.h>
#include "WebView2.h"
#include "headless.h"

#pragma comment(lib, "shlwapi.lib")

static BOOL g_RequestComplete = FALSE;
static char* g_ParsedOutput = NULL;
static HWND g_MessageWindow = NULL;

static wchar_t* g_NormalizedUrl = NULL;
static wchar_t* g_TargetMethod = NULL;
static wchar_t* g_PostPayload = NULL;
static int g_VerboseLogging = 0;

static ICoreWebView2Controller* g_ControllerRef = NULL;
static ICoreWebView2* g_WebViewRef = NULL;

HRESULT STDMETHODCALLTYPE QueryInterfaceStub(void* This, REFIID riid, void** ppv) { 
    *ppv = NULL; 
    return E_NOINTERFACE; 
}
ULONG STDMETHODCALLTYPE AddRefStub(void* This) { return 1; }
ULONG STDMETHODCALLTYPE ReleaseStub(void* This) { return 1; }

// C helper function to escape JSON quotes and backslashes for safe JS string injection
void EscapeJsonForJS(const wchar_t* src, wchar_t* dest, size_t destSize) {
    size_t j = 0;
    for (size_t i = 0; src[i] != L'\0' && j < destSize - 2; i++) {
        if (src[i] == L'"' || src[i] == L'\\') {
            dest[j++] = L'\\';
        }
        dest[j++] = src[i];
    }
    dest[j] = L'\0';
}

HRESULT STDMETHODCALLTYPE OnScriptExtractionDone(ICoreWebView2ExecuteScriptCompletedHandler* This, HRESULT errorCode, LPCWSTR jsonOutput) {
    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] Script execution completed. HRESULT: 0x%08X\n", errorCode);
    
    if (SUCCEEDED(errorCode) && jsonOutput != NULL) {
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, jsonOutput, -1, NULL, 0, NULL, NULL);
        g_ParsedOutput = (char*)malloc(bufferSize);
        WideCharToMultiByte(CP_UTF8, 0, jsonOutput, -1, g_ParsedOutput, bufferSize, NULL, NULL);
        
        if (g_ParsedOutput && g_ParsedOutput[0] == '"') {
            size_t sLen = strlen(g_ParsedOutput);
            if (sLen > 2) {
                memmove(g_ParsedOutput, g_ParsedOutput + 1, sLen - 2);
                g_ParsedOutput[sLen - 2] = '\0';
            }
        }
    }
    g_RequestComplete = TRUE;
    PostQuitMessage(0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE OnNavCompletedHook(ICoreWebView2NavigationCompletedEventHandler* This, ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) {
    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] DOM Navigation Completed. Executing synchronous transmission...\n");

    static ICoreWebView2ExecuteScriptCompletedHandlerVtbl scriptVtbl;
    scriptVtbl.QueryInterface = (void*)QueryInterfaceStub;
    scriptVtbl.AddRef = (void*)AddRefStub;
    scriptVtbl.Release = (void*)ReleaseStub;
    scriptVtbl.Invoke = OnScriptExtractionDone;

    static ICoreWebView2ExecuteScriptCompletedHandler scriptHandler;
    scriptHandler.lpVtbl = &scriptVtbl;

    wchar_t* dataScrapeCode = NULL;

    if (g_TargetMethod && _wcsicmp(g_TargetMethod, L"POST") == 0 && g_NormalizedUrl) {
        wchar_t safePayload[4096] = {0};
        if (g_PostPayload) {
            EscapeJsonForJS(g_PostPayload, safePayload, 4096);
        } else {
            wcscpy_s(safePayload, 5, L"{}");
        }

        size_t codeLen = 512 + wcslen(g_NormalizedUrl) + wcslen(safePayload);
        dataScrapeCode = (wchar_t*)malloc(codeLen * sizeof(wchar_t));

        // Use window.location.href to execute POST directly against the loaded origin context
        swprintf_s(dataScrapeCode, codeLen,
            L"(function() {"
            L"  try {"
            L"    var xhr = new XMLHttpRequest();"
            L"    xhr.open('POST', window.location.href, false);"
            L"    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
            L"    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');"
            L"    var payload = 'json=' + encodeURIComponent('%s');"
            L"    xhr.send(payload);"
            L"    return xhr.responseText;"
            L"  } catch (err) {"
            L"    return 'XHR Error: ' + err.message;"
            L"  }"
            L"})();",
            safePayload
        );
    } else {
        dataScrapeCode = _wcsdup(L"document.documentElement.innerText;");
    }

    ICoreWebView2_ExecuteScript(g_WebViewRef, dataScrapeCode, &scriptHandler);
    free(dataScrapeCode);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE OnControllerReadyHook(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* controller) {
    if (FAILED(result) || !controller) {
        wchar_t errBuf[256];
        swprintf_s(errBuf, 256, L"[ERROR] WebView2 controller creation failed with HRESULT: 0x%08X", result);
        
        int sz = WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, NULL, 0, NULL, NULL);
        g_ParsedOutput = (char*)malloc(sz);
        WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, g_ParsedOutput, sz, NULL, NULL);

        fwprintf(stderr, L"%s\n", errBuf);
        g_RequestComplete = TRUE;
        PostQuitMessage(0);
        return result;
    }

    g_ControllerRef = controller;
    ICoreWebView2Controller_AddRef(g_ControllerRef);

    HRESULT hrCore = ICoreWebView2Controller_get_CoreWebView2(g_ControllerRef, &g_WebViewRef);
    if (FAILED(hrCore) || !g_WebViewRef) {
        wchar_t errBuf[256];
        swprintf_s(errBuf, 256, L"[ERROR] get_CoreWebView2 failed. HR: 0x%08X", hrCore);
        
        int sz = WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, NULL, 0, NULL, NULL);
        g_ParsedOutput = (char*)malloc(sz);
        WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, g_ParsedOutput, sz, NULL, NULL);

        fwprintf(stderr, L"%s\n", errBuf);
        g_RequestComplete = TRUE;
        PostQuitMessage(0);
        return E_FAIL;
    }

    ICoreWebView2_AddRef(g_WebViewRef);

    RECT zeroBounds = { 0, 0, 0, 0 };
    ICoreWebView2Controller_put_Bounds(g_ControllerRef, zeroBounds);
    ICoreWebView2Controller_put_IsVisible(g_ControllerRef, FALSE);

    static ICoreWebView2NavigationCompletedEventHandlerVtbl navVtbl;
    navVtbl.QueryInterface = (void*)QueryInterfaceStub;
    navVtbl.AddRef = (void*)AddRefStub;
    navVtbl.Release = (void*)ReleaseStub;
    navVtbl.Invoke = OnNavCompletedHook;

    static ICoreWebView2NavigationCompletedEventHandler navHandler;
    navHandler.lpVtbl = &navVtbl;

    EventRegistrationToken token = { 0 };
    HRESULT hrSub = ICoreWebView2_add_NavigationCompleted(g_WebViewRef, &navHandler, &token);
    if (FAILED(hrSub)) {
        fwprintf(stderr, L"[ERROR] add_NavigationCompleted failed: 0x%08X\n", hrSub);
        g_ParsedOutput = _strdup("[ERROR] Failed to bind navigation completed event handler.");
        g_RequestComplete = TRUE;
        PostQuitMessage(0);
        return hrSub;
    }

    if (!g_NormalizedUrl) {
        fwprintf(stderr, L"[ERROR] g_NormalizedUrl is NULL before navigation execution.\n");
        g_ParsedOutput = _strdup("[ERROR] Target URL pointer is NULL.");
        g_RequestComplete = TRUE;
        PostQuitMessage(0);
        return E_POINTER;
    }

    // Always navigate directly to the target URL so XHR has a valid document origin
    wchar_t* initialNavTarget = g_NormalizedUrl;

    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] Executing navigation to: %s\n", initialNavTarget);
    
    HRESULT hrNav = ICoreWebView2_Navigate(g_WebViewRef, initialNavTarget);
    if (FAILED(hrNav)) {
        fwprintf(stderr, L"[ERROR] Navigation execution failed with HR: 0x%08X\n", hrNav);
        wchar_t errBuf[128];
        swprintf_s(errBuf, 128, L"[ERROR] Navigation failed: 0x%08X", hrNav);
        int sz = WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, NULL, 0, NULL, NULL);
        g_ParsedOutput = (char*)malloc(sz);
        WideCharToMultiByte(CP_UTF8, 0, errBuf, -1, g_ParsedOutput, sz, NULL, NULL);
        g_RequestComplete = TRUE;
        PostQuitMessage(0);
        return hrNav;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE OnEnvironmentReadyHook(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
    if (FAILED(result) || !env) {
        g_ParsedOutput = _strdup("[ERROR] WebView2 environment creation failed.");
        g_RequestComplete = TRUE;
        return result;
    }

    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] WebView2 Environment Core Ready. Spawning Broker Controller...\n");

    static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctrlVtbl;
    ctrlVtbl.QueryInterface = (void*)QueryInterfaceStub;
    ctrlVtbl.AddRef = (void*)AddRefStub;
    ctrlVtbl.Release = (void*)ReleaseStub;
    ctrlVtbl.Invoke = OnControllerReadyHook;

    static ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ctrlHandler;
    ctrlHandler.lpVtbl = &ctrlVtbl;

    ICoreWebView2Environment_CreateCoreWebView2Controller(env, g_MessageWindow, &ctrlHandler);
    return S_OK;
}

LRESULT CALLBACK SilentWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) { 
    return DefWindowProc(hwnd, msg, wp, lp); 
}

static wchar_t* NormalizeUrl(const wchar_t* rawUrl) {
    if (!rawUrl || wcslen(rawUrl) == 0) return NULL;

    if (StrStrIW(rawUrl, L"http://") == rawUrl || StrStrIW(rawUrl, L"https://") == rawUrl || StrStrIW(rawUrl, L"file://") == rawUrl) {
        size_t len = wcslen(rawUrl) + 1;
        wchar_t* copy = (wchar_t*)malloc(len * sizeof(wchar_t));
        wcscpy_s(copy, len, rawUrl);
        return copy;
    }

    size_t newLen = wcslen(rawUrl) + 9;
    wchar_t* fixedUrl = (wchar_t*)malloc(newLen * sizeof(wchar_t));
    swprintf_s(fixedUrl, newLen, L"https://%s", rawUrl);
    return fixedUrl;
}

__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config) {
    if (!config) {
        return "[ERROR] Configuration structure pointer reference was NULL.";
    }

    if (g_ParsedOutput) {
        free(g_ParsedOutput);
        g_ParsedOutput = NULL;
    }

    g_RequestComplete = FALSE; 
    g_VerboseLogging = config->isVerbose;
    g_NormalizedUrl = NormalizeUrl(config->url);
    
    if (g_TargetMethod) { free(g_TargetMethod); g_TargetMethod = NULL; }
    if (g_PostPayload) { free(g_PostPayload); g_PostPayload = NULL; }

    if (config->method) {
        size_t mLen = wcslen(config->method) + 1;
        g_TargetMethod = (wchar_t*)malloc(mLen * sizeof(wchar_t));
        wcscpy_s(g_TargetMethod, mLen, config->method);
    }

    if (config->postData) {
        size_t pLen = wcslen(config->postData) + 1;
        g_PostPayload = (wchar_t*)malloc(pLen * sizeof(wchar_t));
        wcscpy_s(g_PostPayload, pLen, config->postData);
    }

    if (g_VerboseLogging) {
        fwprintf(stderr, L"[DEBUG] Initializing transaction parameter buffers...\n");
        fwprintf(stderr, L"[DEBUG] Raw Source URL: %s\n", config->url ? config->url : L"NULL");
        fwprintf(stderr, L"[DEBUG] Target Method:  %s\n", g_TargetMethod ? g_TargetMethod : L"GET");
        fwprintf(stderr, L"[DEBUG] Post Payload:   %s\n", g_PostPayload ? g_PostPayload : L"NONE");
    }

    if (!g_NormalizedUrl) {
        return "[ERROR] Invalid or empty URL supplied.";
    }

    HINSTANCE hModInstance = GetModuleHandle(NULL);
    WNDCLASS silentClass = { 0 };
    silentClass.lpfnWndProc = SilentWndProc;
    silentClass.hInstance = hModInstance;
    silentClass.lpszClassName = L"FireGemHeadlessEngine";
    RegisterClass(&silentClass);

    g_MessageWindow = CreateWindowEx(
        0, 
        silentClass.lpszClassName, 
        L"FireGemHost", 
        WS_POPUP, 
        0, 0, 0, 0, 
        NULL, 
        NULL, 
        hModInstance, 
        NULL
    );

    static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl;
    envVtbl.QueryInterface = (void*)QueryInterfaceStub;
    envVtbl.AddRef = (void*)AddRefStub;
    envVtbl.Release = (void*)ReleaseStub;
    envVtbl.Invoke = OnEnvironmentReadyHook;

    static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler envHandler;
    envHandler.lpVtbl = &envVtbl;

    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] Invoking async environment pipeline layer...\n");

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(NULL, NULL, NULL, &envHandler);
    if (FAILED(hr)) {
        if (g_NormalizedUrl) { free(g_NormalizedUrl); g_NormalizedUrl = NULL; }
        if (g_MessageWindow) { DestroyWindow(g_MessageWindow); g_MessageWindow = NULL; }
        return "[ERROR] Call to CreateCoreWebView2EnvironmentWithOptions failed.";
    }

    MSG windowMsg;
    while (!g_RequestComplete) {
        DWORD waitResult = MsgWaitForMultipleObjectsEx(0, NULL, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        
        while (PeekMessage(&windowMsg, NULL, 0, 0, PM_REMOVE)) {
            if (windowMsg.message == WM_QUIT) {
                g_RequestComplete = TRUE;
                break;
            }
            TranslateMessage(&windowMsg);
            DispatchMessage(&windowMsg);
        }
        
        if (waitResult == WAIT_TIMEOUT) {
            Sleep(1);
        }
    }

    if (g_VerboseLogging) fwprintf(stderr, L"[DEBUG] Request completed. Flushing interface pointers...\n");

    if (g_WebViewRef) { ICoreWebView2_Release(g_WebViewRef); g_WebViewRef = NULL; }
    if (g_ControllerRef) { ICoreWebView2Controller_Close(g_ControllerRef); ICoreWebView2Controller_Release(g_ControllerRef); g_ControllerRef = NULL; }
    if (g_MessageWindow) { DestroyWindow(g_MessageWindow); g_MessageWindow = NULL; }
    
    if (g_NormalizedUrl) { free(g_NormalizedUrl); g_NormalizedUrl = NULL; }
    if (g_TargetMethod) { free(g_TargetMethod); g_TargetMethod = NULL; }
    if (g_PostPayload) { free(g_PostPayload); g_PostPayload = NULL; }

    return g_ParsedOutput ? g_ParsedOutput : "[ERROR] Request completed with no output returned.";
}