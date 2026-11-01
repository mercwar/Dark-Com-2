#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <WebView2.h>
#include <wincrypt.h>

// Global DLL state definitions
HWND g_hHiddenWnd = NULL;
ICoreWebView2Controller* g_WebController = NULL;
ICoreWebView2* g_WebView = NULL;
ICoreWebView2Environment* g_WebEnvironment = NULL;
HANDLE g_NavigationEvent = NULL;
char* g_GlobalOutBuffer = NULL;
DWORD g_GlobalOutBufferSize = 0;
volatile LONG g_FetchCompleted = 0;
wchar_t g_CurrentUrl[2300] = {0};

// Forward declarations
static LRESULT CALLBACK HeadlessWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Diagnostic logger stub
void LogSecurityDiagnostic(const char* context, HRESULT hr, const wchar_t* targetUrl) {
    // Optional trace hook
}

// --- SCRIPT EXECUTION COMPLETED HANDLER ---
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
        char ansiPayload[32 * 1024] = {0};
        
        WideCharToMultiByte(CP_UTF8, 0, resultObjectAsJson, (int)len, ansiPayload, sizeof(ansiPayload) - 1, NULL, NULL);

        char* cleanPayload = ansiPayload;
        if (cleanPayload[0] == '"') cleanPayload++;
        size_t cleanLen = strlen(cleanPayload);
        if (cleanLen > 0 && cleanPayload[cleanLen - 1] == '"') {
            cleanPayload[cleanLen - 1] = '\0';
            cleanLen--;
        }

        // Check if payload is JSON/HTML text vs raw base64 binary stream
        int isJsonText = (cleanPayload[0] == '{' || cleanPayload[0] == '[' || strstr(cleanPayload, "<html") != NULL);
        if (!isJsonText) {
            // Unescape json string quotes if returned by JS evaluation
            snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "%s", cleanPayload);
        } else {
            char* preStart = strstr(cleanPayload, "<pre>");
            if (!preStart) preStart = strstr(cleanPayload, "&lt;pre&gt;");

            if (preStart) {
                preStart = strchr(preStart, '>');
                if (preStart) preStart++;
                
                char* preEnd = strstr(preStart, "</pre>");
                if (!preEnd) preEnd = strstr(preStart, "&lt;/pre&gt;");

                if (preEnd) *preEnd = '\0';
                cleanPayload = preStart;
            }
            snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "%s", cleanPayload);
        }
    } else {
        snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "ERROR: Script execution or data extraction failed.");
    }
    InterlockedExchange(&g_FetchCompleted, 1);
    SetEvent(g_NavigationEvent);
    return S_OK;
}

static ICoreWebView2ExecuteScriptCompletedHandlerVtbl g_ScriptCompletedHandlerVtbl = {
    ScriptCompleted_QueryInterface, ScriptCompleted_AddRef, ScriptCompleted_Release, ScriptCompleted_Invoke
};

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
static HRESULT STDMETHODCALLTYPE NavCompleted_Invoke(ICoreWebView2NavigationCompletedEventHandler* This, ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) {
    BOOL isSuccess = TRUE;
    if (args) {
        args->lpVtbl->get_IsSuccess(args, &isSuccess);
    }
    
    LogSecurityDiagnostic("NavCompleted_Invoke", isSuccess ? S_OK : E_FAIL, g_CurrentUrl);

    ScriptCompletedHandler* sHandler = (ScriptCompletedHandler*)malloc(sizeof(ScriptCompletedHandler));
    if (!sHandler) {
        if (g_GlobalOutBuffer) {
            snprintf(g_GlobalOutBuffer, g_GlobalOutBufferSize, "ERROR: Memory allocation failed.");
        }
        InterlockedExchange(&g_FetchCompleted, 1);
        SetEvent(g_NavigationEvent);
        return E_OUTOFMEMORY;
    }
    sHandler->lpVtbl = &g_ScriptCompletedHandlerVtbl;
    sHandler->refCount = 1;

    // Strip any query string (?...) from the URL before checking file extension
    wchar_t cleanUrl[2300];
    wcscpy_s(cleanUrl, 2300, g_CurrentUrl);
    wchar_t* queryPtr = wcschr(cleanUrl, L'?');
    if (queryPtr) *queryPtr = L'\0';

    // Find the last dot in the URL path to correctly isolate the file extension
    wchar_t* dotPtr = wcsrchr(cleanUrl, L'.');
    int isBinary = 0;

    if (dotPtr) {
        if (_wcsicmp(dotPtr, L".cab") == 0 || _wcsicmp(dotPtr, L".exe") == 0 ||
            _wcsicmp(dotPtr, L".png") == 0 || _wcsicmp(dotPtr, L".jpg") == 0 || 
            _wcsicmp(dotPtr, L".jpeg") == 0 || _wcsicmp(dotPtr, L".zip") == 0 || 
            _wcsicmp(dotPtr, L".mid") == 0 || _wcsicmp(dotPtr, L".webp") == 0) {
            isBinary = 1;
        }
    }

    if (isBinary) {
        // Synchronous XHR byte streaming for binary assets
        sender->lpVtbl->ExecuteScript(sender,
            L"(function() {"
            L"  try {"
            L"    let xhr = new XMLHttpRequest();"
            L"    xhr.open('GET', window.location.href, false);"
            L"    xhr.overrideMimeType('text/plain; charset=x-user-defined');"
            L"    xhr.send(null);"
            L"    if (xhr.status !== 200) return '';"
            L"    let text = xhr.responseText;"
            L"    let binary = '';"
            L"    for (let i = 0; i < text.length; i++) {"
            L"      binary += String.fromCharCode(text.charCodeAt(i) & 0xff);"
            L"    }"
            L"    return btoa(binary);"
            L"  } catch(e) { return ''; }"
            L"})();",
            (ICoreWebView2ExecuteScriptCompletedHandler*)sHandler);
    } else {
        sender->lpVtbl->ExecuteScript(sender, 
            L"document.documentElement.outerHTML;", 
            (ICoreWebView2ExecuteScriptCompletedHandler*)sHandler);
    }

    return S_OK;
}
static ICoreWebView2NavigationCompletedEventHandlerVtbl g_NavCompletedHandlerVtbl = {
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
    LogSecurityDiagnostic("Controller_Invoke", result, NULL);
    if (SUCCEEDED(result) && controller != NULL) {
        g_WebController = controller;
        g_WebController->lpVtbl->AddRef(g_WebController);
        g_WebController->lpVtbl->get_CoreWebView2(g_WebController, &g_WebView);
        g_WebController->lpVtbl->put_IsVisible(g_WebController, FALSE);
        
        EventRegistrationToken token;
        NavCompletedHandler* nHandler = (NavCompletedHandler*)malloc(sizeof(NavCompletedHandler));
        if (nHandler) {
            nHandler->lpVtbl = &g_NavCompletedHandlerVtbl;
            nHandler->refCount = 1;
            g_WebView->lpVtbl->add_NavigationCompleted(g_WebView, (ICoreWebView2NavigationCompletedEventHandler*)nHandler, &token);
        }
        SetEvent(g_NavigationEvent);
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl g_ControllerVtbl = {
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
    LogSecurityDiagnostic("Env_Invoke", result, NULL);
    if (SUCCEEDED(result) && env != NULL) {
        g_WebEnvironment = env;
        g_WebEnvironment->lpVtbl->AddRef(g_WebEnvironment);

        EnvironmentHandler* impl = (EnvironmentHandler*)This;
        ControllerHandler* cHandler = (ControllerHandler*)malloc(sizeof(ControllerHandler));
        if (cHandler) {
            cHandler->lpVtbl = &g_ControllerVtbl;
            cHandler->refCount = 1;
            env->lpVtbl->CreateCoreWebView2Controller(env, impl->hWndParent, (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)cHandler);
        }
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl g_EnvVtbl = {
    Env_QueryInterface, Env_AddRef, Env_Release, Env_Invoke
};

static void InitializeHeadlessEngine(void) {
    if (g_WebView) return;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = HeadlessWndProc;
    wcex.hInstance     = GetModuleHandle(NULL);
    wcex.lpszClassName = L"DarkComHeadlessClass";
    RegisterClassExW(&wcex);

    g_hHiddenWnd = CreateWindowExW(0, L"DarkComHeadlessClass", L"DarkComHeadless",
        WS_OVERLAPPED, 0, 0, 100, 100, NULL, NULL, wcex.hInstance, NULL);

    g_NavigationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_NavigationEvent);

    wchar_t exePathBuffer[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuffer, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePathBuffer, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    wchar_t cachePath[MAX_PATH];
    swprintf_s(cachePath, MAX_PATH, L"%s\\browser_cache", exePathBuffer);

    EnvironmentHandler* eHandler = (EnvironmentHandler*)malloc(sizeof(EnvironmentHandler));
    if (eHandler) {
        eHandler->lpVtbl = &g_EnvVtbl;
        eHandler->refCount = 1;
        eHandler->hWndParent = g_hHiddenWnd;

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(NULL, cachePath, NULL, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)eHandler);
        LogSecurityDiagnostic("CreateCoreWebView2EnvironmentWithOptions", hr, NULL);
    }

    DWORD startTime = GetTickCount();
    MSG msg;
    while (!g_WebView && (GetTickCount() - startTime < 10000)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
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

// --- DLL EXPORTS ---
__declspec(dllexport) int RunHeadlessRequest(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize) {
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

    MultiByteToWideChar(CP_UTF8, 0, url, -1, g_CurrentUrl, (int)(sizeof(g_CurrentUrl) / sizeof(wchar_t)) - 1);
    
    HRESULT hr = g_WebView->lpVtbl->Navigate(g_WebView, g_CurrentUrl);
    if (FAILED(hr)) {
        snprintf(outBuffer, outBufferSize, "ERROR: Navigation failure (0x%08X).", (unsigned int)hr);
        return -1;
    }

    DWORD startTime = GetTickCount();
    MSG msg;
    while (g_FetchCompleted == 0 && (GetTickCount() - startTime < 15000)) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }

if (g_FetchCompleted == 0) {
        snprintf(outBuffer, outBufferSize, "ERROR: Extraction timed out within 15 seconds.");
        return -2;
    }

    // Return the actual number of bytes written to the buffer
    return (int)strlen(outBuffer);

}

__declspec(dllexport) void RunHeadlessShutdown(void) {
    if (g_WebView) {
        g_WebView->lpVtbl->Release(g_WebView);
        g_WebView = NULL;
    }
    if (g_WebController) {
        g_WebController->lpVtbl->Release(g_WebController);
        g_WebController = NULL;
    }
    if (g_WebEnvironment) {
        g_WebEnvironment->lpVtbl->Release(g_WebEnvironment);
        g_WebEnvironment = NULL;
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