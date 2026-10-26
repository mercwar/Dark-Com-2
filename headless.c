/* ============================================================================
   headless.c - WebView2 Headless Execution Engine (Pure C Version)
   ============================================================================ */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <webview2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headless.h"

typedef struct {
    char responseBuffer[65536];
    volatile LONG isComplete;
    volatile LONG isNavComplete;
} HeadlessExecutionContext;

static __declspec(thread) HeadlessExecutionContext* g_CurrentContext = NULL;

static HRESULT STDMETHODCALLTYPE Generic_QueryInterface(void* This, REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2WebMessageReceivedEventHandler) || IsEqualIID(riid, &IID_ICoreWebView2NavigationCompletedEventHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Generic_AddRef(void* This) { return 1; }
static ULONG STDMETHODCALLTYPE Generic_Release(void* This) { return 1; }

// Web Message Received Handler
static HRESULT STDMETHODCALLTYPE OnWebMessageReceived_Invoke(
    ICoreWebView2WebMessageReceivedEventHandler* This,
    ICoreWebView2* sender,
    ICoreWebView2WebMessageReceivedEventArgs* args)
{
    LPWSTR webMessageStr = NULL;
    if (SUCCEEDED(ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(args, &webMessageStr)) && webMessageStr) {
        if (g_CurrentContext) {
            WideCharToMultiByte(CP_UTF8, 0, webMessageStr, -1, g_CurrentContext->responseBuffer, sizeof(g_CurrentContext->responseBuffer) - 1, NULL, NULL);
            InterlockedExchange(&g_CurrentContext->isComplete, 1);
        }
        CoTaskMemFree(webMessageStr);
    }
    return S_OK;
}

// Navigation Completed Handler
static HRESULT STDMETHODCALLTYPE OnNavigationCompleted_Invoke(
    ICoreWebView2NavigationCompletedEventHandler* This,
    ICoreWebView2* sender,
    ICoreWebView2NavigationCompletedEventArgs* args)
{
    if (g_CurrentContext) {
        InterlockedExchange(&g_CurrentContext->isNavComplete, 1);
    }
    return S_OK;
}

static void EscapeForJsString(const char* src, char* dest, size_t destSize) {
    size_t di = 0;
    for (size_t si = 0; src[si] != '\0' && di < destSize - 2; si++) {
        char c = src[si];
        if (c == '\\' || c == '\'' || c == '\"') {
            if (di < destSize - 3) {
                dest[di++] = '\\';
                dest[di++] = c;
            }
        } else if (c == '\n') {
            if (di < destSize - 3) {
                dest[di++] = '\\';
                dest[di++] = 'n';
            }
        } else if (c == '\r') {
            if (di < destSize - 3) {
                dest[di++] = '\\';
                dest[di++] = 'r';
            }
        } else {
            dest[di++] = c;
        }
    }
    dest[di] = '\0';
}

__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config) {
    static __declspec(thread) HeadlessExecutionContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    g_CurrentContext = &ctx;

    if (!config || !config->webView || !config->url) {
        return "{\"status\":\"error\",\"message\":\"Invalid configuration or uninitialized WebView2 instance.\"}";
    }

    // 1. Extract Origin/Base URL for pre-flight navigation so fetch() runs in a valid origin context
    wchar_t originUrl[1024] = {0};
    const wchar_t* urlPtr = config->url;
    if (wcsncmp(urlPtr, L"http://", 7) == 0 || wcsncmp(urlPtr, L"https://", 8) == 0) {
        const wchar_t* pathStart = wcschr(urlPtr + 8, L'/');
        if (pathStart) {
            size_t len = pathStart - urlPtr;
            wcsncpy_s(originUrl, 1024, urlPtr, len);
        } else {
            wcscpy_s(originUrl, 1024, urlPtr);
        }
    } else {
        wcscpy_s(originUrl, 1024, L"http://localhost");
    }

    // Register Navigation Completed Handler
    ICoreWebView2NavigationCompletedEventHandlerVtbl navVtbl = {
        (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2NavigationCompletedEventHandler*, REFIID, void**))Generic_QueryInterface,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2NavigationCompletedEventHandler*))Generic_AddRef,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2NavigationCompletedEventHandler*))Generic_Release,
        OnNavigationCompleted_Invoke
    };
    ICoreWebView2NavigationCompletedEventHandler navHandler = { &navVtbl };
    EventRegistrationToken navToken;
    ICoreWebView2_add_NavigationCompleted(config->webView, &navHandler, &navToken);

    // Navigate WebView to target origin first to establish valid document security context
    HRESULT hr = ICoreWebView2_Navigate(config->webView, originUrl);
    if (FAILED(hr)) {
        ICoreWebView2_remove_NavigationCompleted(config->webView, navToken);
        snprintf(ctx.responseBuffer, sizeof(ctx.responseBuffer), "{\"status\":\"error\",\"message\":\"Navigation HRESULT 0x%08X\"}", (unsigned int)hr);
        return ctx.responseBuffer;
    }

    // Wait for navigation to complete
    while (!InterlockedOr(&ctx.isNavComplete, 0)) {
        MsgWaitForMultipleObjectsEx(0, NULL, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    ICoreWebView2_remove_NavigationCompleted(config->webView, navToken);

    // 2. Register Web Message Received Handler for fetch response bridging
    ICoreWebView2WebMessageReceivedEventHandlerVtbl msgVtbl = {
        (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2WebMessageReceivedEventHandler*, REFIID, void**))Generic_QueryInterface,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2WebMessageReceivedEventHandler*))Generic_AddRef,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2WebMessageReceivedEventHandler*))Generic_Release,
        OnWebMessageReceived_Invoke
    };
    ICoreWebView2WebMessageReceivedEventHandler msgHandler = { &msgVtbl };
    EventRegistrationToken msgToken;
    ICoreWebView2_add_WebMessageReceived(config->webView, &msgHandler, &msgToken);

    const wchar_t* ctype = config->contentType ? config->contentType : L"application/x-www-form-urlencoded";
    if (config->postData && (config->postData[0] == L'{' || config->postData[0] == L'[')) {
        if (!config->contentType) {
            ctype = L"application/json";
        }
    }

    char utf8PostData[16384] = {0};
    char escapedPostData[32768] = {0};
    if (config->postData && wcslen(config->postData) > 0) {
        WideCharToMultiByte(CP_UTF8, 0, config->postData, -1, utf8PostData, sizeof(utf8PostData) - 1, NULL, NULL);
        EscapeForJsString(utf8PostData, escapedPostData, sizeof(escapedPostData));
    }

    char utf8Url[2048] = {0};
    char utf8Method[32] = {0};
    char utf8Ctype[128] = {0};
    
    WideCharToMultiByte(CP_UTF8, 0, config->url ? config->url : L"", -1, utf8Url, sizeof(utf8Url) - 1, NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, config->method ? config->method : L"GET", -1, utf8Method, sizeof(utf8Method) - 1, NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, ctype, -1, utf8Ctype, sizeof(utf8Ctype) - 1, NULL, NULL);

    // Construct JS script to execute fetch() now that origin security context is valid
    wchar_t jsScript[49152];
    if (config->postData && wcslen(config->postData) > 0) {
        swprintf_s(jsScript, 49152,
            L"(async function() {"
            L"    try {"
            L"        let res = await fetch('%S', {"
            L"            method: '%S',"
            L"            mode: 'cors',"
            L"            credentials: 'include',"
            L"            headers: {"
            L"                'Content-Type': '%S',"
            L"                'X-Requested-With': 'XMLHttpRequest'"
            L"            },"
            L"            body: '%S'"
            L"        });"
            L"        let txt = await res.text();"
            L"        let hdrs = {};"
            L"        for (let [k, v] of res.headers.entries()) { hdrs[k] = v; }"
            L"        window.chrome.webview.postMessage(JSON.stringify({"
            L"            status: res.status,"
            L"            statusText: res.statusText,"
            L"            headers: hdrs,"
            L"            body: txt"
            L"        }));"
            L"    } catch(e) {"
            L"        window.chrome.webview.postMessage(JSON.stringify({status: 'error', message: e.toString()}));"
            L"    }"
            L"})();",
            utf8Url, utf8Method, utf8Ctype, escapedPostData
        );
    } else {
        swprintf_s(jsScript, 49152,
            L"(async function() {"
            L"    try {"
            L"        let res = await fetch('%S', {"
            L"            method: '%S',"
            L"            mode: 'cors',"
            L"            credentials: 'include',"
            L"            headers: {"
            L"                'X-Requested-With': 'XMLHttpRequest'"
            L"            }"
            L"        });"
            L"        let txt = await res.text();"
            L"        let hdrs = {};"
            L"        for (let [k, v] of res.headers.entries()) { hdrs[k] = v; }"
            L"        window.chrome.webview.postMessage(JSON.stringify({"
            L"            status: res.status,"
            L"            statusText: res.statusText,"
            L"            headers: hdrs,"
            L"            body: txt"
            L"        }));"
            L"    } catch(e) {"
            L"        window.chrome.webview.postMessage(JSON.stringify({status: 'error', message: e.toString()}));"
            L"    }"
            L"})();",
            utf8Url, utf8Method
        );
    }

    static ICoreWebView2ExecuteScriptCompletedHandlerVtbl execVtbl = {
        (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2ExecuteScriptCompletedHandler*, REFIID, void**))Generic_QueryInterface,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2ExecuteScriptCompletedHandler*))Generic_AddRef,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2ExecuteScriptCompletedHandler*))Generic_Release,
        (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2ExecuteScriptCompletedHandler*, HRESULT, LPCWSTR))Generic_AddRef
    };
    ICoreWebView2ExecuteScriptCompletedHandler execHandler = { &execVtbl };

    hr = ICoreWebView2_ExecuteScript(config->webView, jsScript, &execHandler);
    if (FAILED(hr)) {
        snprintf(ctx.responseBuffer, sizeof(ctx.responseBuffer), "{\"status\":\"error\",\"message\":\"ICoreWebView2_ExecuteScript HRESULT 0x%08X\"}", (unsigned int)hr);
        ICoreWebView2_remove_WebMessageReceived(config->webView, msgToken);
        return ctx.responseBuffer;
    }

    // Pump message loop until postMessage delivers the payload
    while (!InterlockedOr(&ctx.isComplete, 0)) {
        MsgWaitForMultipleObjectsEx(0, NULL, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    ICoreWebView2_remove_WebMessageReceived(config->webView, msgToken);
    g_CurrentContext = NULL;
    return ctx.responseBuffer;
}