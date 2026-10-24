#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <oleacc.h>
#include <stdio.h>
#include <stdlib.h>
#include "WebView2.h"
#include "headless.h"

static BOOL g_RequestComplete = FALSE;
static char* g_ParsedOutput = NULL;
static HWND g_MessageWindow = NULL;

static const wchar_t* g_UrlPtr = NULL;
static const wchar_t* g_PayloadPtr = NULL;

static ICoreWebView2Controller* g_ControllerRef = NULL;
static ICoreWebView2* g_WebViewRef = NULL;

// Mandated COM interface stubs required for pure C linkage
HRESULT STDMETHODCALLTYPE QueryInterfaceStub(void* This, REFIID riid, void** ppv) { 
    *ppv = NULL; 
    return E_NOINTERFACE; 
}
ULONG STDMETHODCALLTYPE AddRefStub(void* This) { return 1; }
ULONG STDMETHODCALLTYPE ReleaseStub(void* This) { return 1; }

HRESULT STDMETHODCALLTYPE OnScriptExtractionDone(ICoreWebView2ExecuteScriptCompletedHandler* This, HRESULT errorCode, LPCWSTR jsonOutput) {
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
    static ICoreWebView2ExecuteScriptCompletedHandlerVtbl scriptVtbl;
    scriptVtbl.QueryInterface = (void*)QueryInterfaceStub;
    scriptVtbl.AddRef = (void*)AddRefStub;
    scriptVtbl.Release = (void*)ReleaseStub;
    scriptVtbl.Invoke = OnScriptExtractionDone;

    static ICoreWebView2ExecuteScriptCompletedHandler scriptHandler;
    scriptHandler.lpVtbl = &scriptVtbl;

    wchar_t* dataScrapeCode = L"document.documentElement.innerText;";
    ICoreWebView2_ExecuteScript(g_WebViewRef, dataScrapeCode, &scriptHandler);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE OnControllerReadyHook(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* controller) {
    if (SUCCEEDED(result)) {
        g_ControllerRef = controller;
        ICoreWebView2Controller_get_CoreWebView2(g_ControllerRef, &g_WebViewRef);

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

        ICoreWebView2_add_NavigationCompleted(g_WebViewRef, &navHandler, NULL);
        ICoreWebView2_Navigate(g_WebViewRef, g_UrlPtr);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE OnEnvironmentReadyHook(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
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

const char* RunHeadlessRequest(const wchar_t* url, const wchar_t* postData) {
    g_RequestComplete = FALSE; 
    g_UrlPtr = url; 
    g_PayloadPtr = postData;

    HINSTANCE hModInstance = GetModuleHandle(NULL);
    WNDCLASS silentClass = { 0 };
    silentClass.lpfnWndProc = SilentWndProc;
    silentClass.hInstance = hModInstance;
    silentClass.lpszClassName = L"FireGemHeadlessEngine";
    RegisterClass(&silentClass);

    g_MessageWindow = CreateWindowEx(0, silentClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hModInstance, NULL);

    static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl;
    envVtbl.QueryInterface = (void*)QueryInterfaceStub;
    envVtbl.AddRef = (void*)AddRefStub;
    envVtbl.Release = (void*)ReleaseStub;
    envVtbl.Invoke = OnEnvironmentReadyHook;

    static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler envHandler;
    envHandler.lpVtbl = &envVtbl;

    CreateCoreWebView2EnvironmentWithOptions(NULL, NULL, NULL, &envHandler);

    MSG windowMsg;
    while (!g_RequestComplete && GetMessage(&windowMsg, NULL, 0, 0)) { 
        TranslateMessage(&windowMsg); 
        DispatchMessage(&windowMsg); 
    }

    if (g_WebViewRef) { ICoreWebView2_Release(g_WebViewRef); g_WebViewRef = NULL; }
    if (g_ControllerRef) { ICoreWebView2Controller_Close(g_ControllerRef); ICoreWebView2Controller_Release(g_ControllerRef); g_ControllerRef = NULL; }
    if (g_MessageWindow) { DestroyWindow(g_MessageWindow); g_MessageWindow = NULL; }

    return g_ParsedOutput;
}
