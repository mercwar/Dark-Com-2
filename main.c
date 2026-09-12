#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <WebView2.h>

// Core UI control identifiers
#define IDI_APP_ICON 101
#define IDC_URL_EDIT 101
#define IDC_GO_BUTTON 102

// Global UI and interface handles 
HWND g_hWndParent = NULL;
HWND g_hWndUrlEdit = NULL;
HWND g_hWndGoButton = NULL;
ICoreWebView2Controller* g_WebController = NULL;
ICoreWebView2* g_WebView = NULL;

// Sizing and placement of the Nav Bar and WebView2 Control together
void ResizeUI(void) {
    if (!g_hWndParent) return;
    
    RECT rect;
    GetClientRect(g_hWndParent, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    int navBarHeight = 40;
    int buttonWidth = 60;
    int padding = 5;
    
    int editWidth = windowWidth - buttonWidth - (padding * 3);
    MoveWindow(g_hWndUrlEdit, padding, padding, editWidth, navBarHeight - (padding * 2), TRUE);
    MoveWindow(g_hWndGoButton, padding + editWidth + padding, padding, buttonWidth, navBarHeight - (padding * 2), TRUE);
    
    if (g_WebController != NULL) {
        RECT webBounds;
        webBounds.left = 0;
        webBounds.top = navBarHeight;
        webBounds.right = windowWidth;
        webBounds.bottom = windowHeight;
        g_WebController->lpVtbl->put_Bounds(g_WebController, webBounds);
    }
}

// Trigger browser routing based on address bar content
void TriggerNavigation(void) {
    if (g_WebView == NULL || g_hWndUrlEdit == NULL) return;
    
    // FIX: Allocated proper array buffer lengths instead of a single char
    wchar_t urlBuffer[2048] = {0};
    GetWindowTextW(g_hWndUrlEdit, urlBuffer, 2048);
    
    wchar_t targetUrl[2100] = {0};
    // Auto prepend https protocol if it is missing
    if (wcsstr(urlBuffer, L"http://") != urlBuffer && wcsstr(urlBuffer, L"https://") != urlBuffer) {
        swprintf(targetUrl, 2100, L"https://%s", urlBuffer);
    } else {
        wcsncpy(targetUrl, urlBuffer, 2100);
    }
    
    g_WebView->lpVtbl->Navigate(g_WebView, targetUrl);
}

// --- COM CALLBACK IMPLEMENTATIONS FOR PURE C ---

typedef struct {
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler lpVtbl;
    LONG refCount;
} ControllerHandler;

HRESULT STDMETHODCALLTYPE Controller_QueryInterface(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE Controller_AddRef(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This) {
    ControllerHandler* impl = (ControllerHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
ULONG STDMETHODCALLTYPE Controller_Release(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This) {
    ControllerHandler* impl = (ControllerHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}
HRESULT STDMETHODCALLTYPE Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This, HRESULT result, ICoreWebView2Controller* controller) {
    if (SUCCEEDED(result) && controller != NULL) {
        g_WebController = controller;
        g_WebController->lpVtbl->AddRef(g_WebController);
        g_WebController->lpVtbl->get_CoreWebView2(g_WebController, &g_WebView);
        
        // FIX: Ensure visibility is explicitly set to TRUE or the page renders transparent
        g_WebController->lpVtbl->put_IsVisible(g_WebController, TRUE);
        
        ResizeUI();
        TriggerNavigation();
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl controllerVtbl = {
    Controller_QueryInterface, Controller_AddRef, Controller_Release, Controller_Invoke
};

typedef struct {
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler lpVtbl;
    LONG refCount;
    HWND hWndParent;
} EnvironmentHandler;

HRESULT STDMETHODCALLTYPE Env_QueryInterface(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE Env_AddRef(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This) {
    EnvironmentHandler* impl = (EnvironmentHandler*)This;
    return InterlockedIncrement(&impl->refCount);
}
ULONG STDMETHODCALLTYPE Env_Release(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This) {
    EnvironmentHandler* impl = (EnvironmentHandler*)This;
    LONG rc = InterlockedDecrement(&impl->refCount);
    if (rc == 0) free(impl);
    return rc;
}
HRESULT STDMETHODCALLTYPE Env_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This, HRESULT result, ICoreWebView2Environment* env) {
    if (SUCCEEDED(result) && env != NULL) {
        EnvironmentHandler* impl = (EnvironmentHandler*)This;
        ControllerHandler* cHandler = (ControllerHandler*)malloc(sizeof(ControllerHandler));
        cHandler->lpVtbl.lpVtbl = &controllerVtbl;
        cHandler->refCount = 1;
        
        env->lpVtbl->CreateCoreWebView2Controller(env, impl->hWndParent, (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)cHandler);
    }
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = {
    Env_QueryInterface, Env_AddRef, Env_Release, Env_Invoke
};

// --- WINDOWS ENGINE MESSAGE ROUTING ---

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            g_hWndParent = hWnd;
            HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
            
            g_hWndUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"https://mercwar01.byethost3.com",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0, 0, 0, 0, hWnd, (HMENU)IDC_URL_EDIT, hInstance, NULL);
            
            g_hWndGoButton = CreateWindowW(L"BUTTON", L"Go",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_GO_BUTTON, hInstance, NULL);
            
            EnvironmentHandler* eHandler = (EnvironmentHandler*)malloc(sizeof(EnvironmentHandler));
            eHandler->lpVtbl.lpVtbl = &envVtbl;
            eHandler->refCount = 1;
            eHandler->hWndParent = hWnd;
            
            CreateCoreWebView2EnvironmentWithOptions(NULL, NULL, NULL, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)eHandler);
            break;
        }
        case WM_SIZE:
            ResizeUI();
            break;
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_GO_BUTTON) {
                TriggerNavigation();
            }
            break;
        }
        case WM_DESTROY:
            if (g_WebView) g_WebView->lpVtbl->Release(g_WebView);
            if (g_WebController) g_WebController->lpVtbl->Release(g_WebController);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // FIX: Initialize COM library architecture for the main UI thread execution context
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.lpszClassName  = L"WebView2BrowserClass";
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    
    // ADDED: Load and assign the custom application icons
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wcex.hIconSm        = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    
    RegisterClassExW(&wcex);
    
    HWND hWnd = CreateWindowW(L"WebView2BrowserClass", L"Stargate Dark-Com v2 ", 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, 
        NULL, NULL, hInstance, NULL);
        
    if (!hWnd) return FALSE;
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetFocus() == g_hWndUrlEdit) {
            TriggerNavigation();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    return (int)msg.wParam;
}
