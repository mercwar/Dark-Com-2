#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <WebView2.h>
#include <wincrypt.h>

// Global DLL state
static HWND g_hHiddenWnd = NULL;
static ICoreWebView2Controller* g_WebController = NULL;
static ICoreWebView2* g_WebView = NULL;
static HANDLE g_NavigationEvent = NULL;
static char* g_GlobalOutBuffer = NULL;
static DWORD g_GlobalOutBufferSize = 0;
static volatile LONG g_FetchCompleted = 0;

// Forward declarations
static LRESULT CALLBACK HeadlessWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// --- NAVIGATION COMPLETED EVENT HANDLER ---

typedef struct {
    ICoreWebView2NavigationCompletedEventHandlerVtbl* lpVtbl;
    LONG refCount;
} NavCompletedHandler;

static HRESULT STDMETHODCALLTYPE NavCompleted_QueryInterface(ICoreWebView2NavigationCompletedEventHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2NavigationCompletedEventHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE NavCompleted_AddRef(ICoreWebView2NavigationCompletedEventHandler* This) {
    NavCompletedHandler* impl = (NavCompletedHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
static ULONG STDMETHODCALLTYPE NavCompleted_Release(ICoreWebView2NavigationCompletedEventHandler* This) {
    NavCompletedHandler* impl = (NavCompletedHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}

// Callback when JavaScript script execution (getting outerHTML) finishes
typedef struct {
    ICoreWebView2ExecuteScriptCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
} ScriptCompletedHandler;

static HRESULT STDMETHODCALLTYPE ScriptCompleted_QueryInterface(ICoreWebView2ExecuteScriptCompletedHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2ExecuteScriptCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE ScriptCompleted_AddRef(ICoreWebView2ExecuteScriptCompletedHandler* This) {
    ScriptCompletedHandler* impl = (ScriptCompletedHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
static ULONG STDMETHODCALLTYPE ScriptCompleted_Release(ICoreWebView2ExecuteScriptCompletedHandler* This) {
    ScriptCompletedHandler* impl = (ScriptCompletedHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}
static HRESULT STDMETHODCALLTYPE ScriptCompleted_Invoke(ICoreWebView2ExecuteScriptCompletedHandler* This, HRESULT errorCode, LPCWSTR resultObjectAsJson) {
    if (SUCCEEDED(errorCode) && resultObjectAsJson && g_GlobalOutBuffer) {
        size_t len = wcslen(resultObjectAsJson);
        char ansiHtml[16 * 1024] = {0};
        
        WideCharToMultiByte(CP_UTF8, 0, resultObjectAsJson, (int)len, ansiHtml, sizeof(ansiHtml) - 1, NULL, NULL);

        char* cleanHtml = ansiHtml;
        if (cleanHtml[0] == '"') cleanHtml++;
        size_t cleanLen = strlen(cleanHtml);
        if (cleanLen > 0 && cleanHtml[cleanLen - 1] == '"') {
            cleanHtml[cleanLen - 1] = '\0';
            cleanLen--;
        }

        DWORD b64Len = g_GlobalOutBufferSize;
        if (CryptBinaryToStringA((BYTE*)cleanHtml, (DWORD)cleanLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, g_GlobalOutBuffer, &b64Len)) {
            // Success
        } else {
            snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "%s", cleanHtml);
        }
    } else {
        snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "ERROR: Script execution or HTML extraction failed.");
    }
    InterlockedExchange(&g_FetchCompleted, 1);
    SetEvent(g_NavigationEvent);
    return S_OK;
}

static ICoreWebView2ExecuteScriptCompletedHandlerVtbl scriptVtbl = {
    ScriptCompleted_QueryInterface, ScriptCompleted_AddRef, ScriptCompleted_Release, ScriptCompleted_Invoke
};

static HRESULT STDMETHODCALLTYPE NavCompleted_Invoke(ICoreWebView2NavigationCompletedEventHandler* This, ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) {
    ScriptCompletedHandler* sHandler = (ScriptCompletedHandler*)malloc(sizeof(ScriptCompletedHandler));
    sHandler->lpVtbl = &scriptVtbl;
    sHandler->refCount = 1;

    sender->lpVtbl->ExecuteScript(sender, L"document.documentElement.outerHTML;", (ICoreWebView2ExecuteScriptCompletedHandler*)sHandler);
    return S_OK;
}

static ICoreWebView2NavigationCompletedEventHandlerVtbl navCompletedVtbl = {
    NavCompleted_QueryInterface, NavCompleted_AddRef, NavCompleted_Release, NavCompleted_Invoke
};

// --- ENVIRONMENT & CONTROLLER COM CALLBACKS ---

typedef struct {
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
} ControllerHandler;

static HRESULT STDMETHODCALLTYPE Controller_QueryInterface(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE Controller_AddRef(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This) {
    ControllerHandler* impl = (ControllerHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
static ULONG STDMETHODCALLTYPE Controller_Release(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This) {
    ControllerHandler* impl = (ControllerHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}
static HRESULT STDMETHODCALLTYPE Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* controller) {
    if (SUCCEEDED(result) && controller != NULL) {
        g_WebController = controller;
        g_WebController->lpVtbl->AddRef(g_WebController);
        g_WebController->lpVtbl->get_CoreWebView2(g_WebController, &g_WebView);
        g_WebController->lpVtbl->put_IsVisible(g_WebController, FALSE);
        SetEvent(g_NavigationEvent);
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl controllerVtbl = {
    Controller_QueryInterface, Controller_AddRef, Controller_Release, Controller_Invoke
};

typedef struct {
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl* lpVtbl;
    LONG refCount;
    HWND hWndParent;
} EnvironmentHandler;

static HRESULT STDMETHODCALLTYPE Env_QueryInterface(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE Env_AddRef(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This) {
    EnvironmentHandler* impl = (EnvironmentHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
static ULONG STDMETHODCALLTYPE Env_Release(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This) {
    EnvironmentHandler* impl = (EnvironmentHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}
static HRESULT STDMETHODCALLTYPE Env_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
    if (SUCCEEDED(result) && env != NULL) {
        EnvironmentHandler* impl = (EnvironmentHandler*)This;
        ControllerHandler* cHandler = (ControllerHandler*)malloc(sizeof(ControllerHandler));
        cHandler->lpVtbl = &controllerVtbl;
        cHandler->refCount = 1;
        
        env->lpVtbl->CreateCoreWebView2Controller(env, impl->hWndParent, (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)cHandler);
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = {
    Env_QueryInterface, Env_AddRef, Env_Release, Env_Invoke
};

static void InitializeHeadlessEngine(void) {
    if (g_WebView) return;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = HeadlessWndProc;
    wcex.hInstance     = GetModuleHandle(NULL);
    wcex.lpszClassName = L"DarkComHeadlessClass";
    RegisterClassExW(&wcex);

    g_hHiddenWnd = CreateWindowExW(0, L"DarkComHeadlessClass", L"DarkComHeadless",
        WS_OVERLAPPED, 0, 0, 100, 100, NULL, NULL, wcex.hInstance, NULL);

    g_NavigationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_NavigationEvent);

    EnvironmentHandler* eHandler = (EnvironmentHandler*)malloc(sizeof(EnvironmentHandler));
    eHandler->lpVtbl = &envVtbl;
    eHandler->refCount = 1;
    eHandler->hWndParent = g_hHiddenWnd;

    CreateCoreWebView2EnvironmentWithOptions(NULL, NULL, NULL, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)eHandler);

    DWORD startTime = GetTickCount();
    MSG msg;
    while (!g_WebView && (GetTickCount() - startTime < 10000)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(50);
    }
}

static LRESULT CALLBACK HeadlessWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// --- DLL EXPORTS MATCHING THE DEF FILE ---

int RunHeadlessRequest(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
    if (!g_WebView) {
        InitializeHeadlessEngine();
    }

    if (!g_WebView) {
        snprintf(outBuffer, outBufferSize, "ERROR: WebView2 initialization failed.");
        return -1;
    }

    g_GlobalOutBuffer = outBuffer;
    g_GlobalOutBufferSize = outBufferSize;
    InterlockedExchange(&g_FetchCompleted, 0);
    ResetEvent(g_NavigationEvent);

    wchar_t wUrl[2300] = {0};
    MultiByteToWideChar(CP_ACP, 0, url, -1, wUrl, 2300);

    EventRegistrationToken token;
    NavCompletedHandler* nHandler = (NavCompletedHandler*)malloc(sizeof(NavCompletedHandler));
    nHandler->lpVtbl = &navCompletedVtbl;
    nHandler->refCount = 1;

    g_WebView->lpVtbl->add_NavigationCompleted(g_WebView, (ICoreWebView2NavigationCompletedEventHandler*)nHandler, &token);

    HRESULT hr = g_WebView->lpVtbl->Navigate(g_WebView, wUrl);
    if (FAILED(hr)) {
        snprintf(outBuffer, outBufferSize, "ERROR: Navigation call failed.");
        return -1;
    }

    DWORD startTime = GetTickCount();
    MSG msg;
    while (!g_FetchCompleted && (GetTickCount() - startTime < 15000)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(50);
    }

    g_WebView->lpVtbl->remove_NavigationCompleted(g_WebView, token);

    if (!g_FetchCompleted) {
        snprintf(outBuffer, outBufferSize, "ERROR: Navigation timed out.");
        return -1;
    }

    return (int)strlen(outBuffer);
}

void RunHeadlessShutdown(void) {
    if (g_WebView) {
        g_WebView->lpVtbl->Release(g_WebView);
        g_WebView = NULL;
    }
    if (g_WebController) {
        g_WebController->lpVtbl->Release(g_WebController);
        g_WebController = NULL;
    }
    if (g_NavigationEvent) {
        CloseHandle(g_NavigationEvent);
        g_NavigationEvent = NULL;
    }
    if (g_hHiddenWnd) {
        DestroyWindow(g_hHiddenWnd);
        g_hHiddenWnd = NULL;
    }
    CoUninitialize();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            RunHeadlessShutdown();
            break;
    }
    return TRUE;
}