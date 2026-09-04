// AVIS - Integrated Window Form Browser Controller with Navigation Bar
// AIFVS-ARTIFACT
// File: main.cpp
#define IDI_APP_ICON 101

#include <windows.h>
#include <stdlib.h>
#include <wrl.h>
#include <string>
#include "WebView2.h"

using namespace Microsoft::WRL;

// Core UI control identifiers
#define IDC_URL_EDIT   101
#define IDC_GO_BUTTON  102

// Global UI handles
HWND g_hWndParent = NULL;
HWND g_hWndUrlEdit = NULL;
HWND g_hWndGoButton = NULL;

ComPtr<ICoreWebView2Controller> g_WebController;
ComPtr<ICoreWebView2> g_WebView;

// Handles sizing and placement of the Nav Bar and WebView2 Control together
void ResizeUI() {
    if (!g_hWndParent) return;

    RECT rect;
    GetClientRect(g_hWndParent, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Define Nav Bar layout metrics
    int navBarHeight = 40;
    int buttonWidth = 60;
    int padding = 5;

    // Place the URL textbox and Go button at the top
    int editWidth = windowWidth - buttonWidth - (padding * 3);
    MoveWindow(g_hWndUrlEdit, padding, padding, editWidth, navBarHeight - (padding * 2), TRUE);
    MoveWindow(g_hWndGoButton, padding + editWidth + padding, padding, buttonWidth, navBarHeight - (padding * 2), TRUE);

    // Shift the main WebView2 window downward to sit right under the Nav Bar
    if (g_WebController != nullptr) {
        RECT webBounds;
        webBounds.left = 0;
        webBounds.top = navBarHeight;
        webBounds.right = windowWidth;
        webBounds.bottom = windowHeight;
        g_WebController->put_Bounds(webBounds);
    }
}

// Function to trigger browser routing based on what is typed in the text box
void TriggerNavigation() {
    if (g_WebView == nullptr || g_hWndUrlEdit == NULL) return;

    wchar_t urlBuffer[2048];
    GetWindowTextW(g_hWndUrlEdit, urlBuffer, 2048);
    std::wstring targetUrl(urlBuffer);

    // Auto-prepend https:// protocol if it is missing so the engine routes properly
    if (targetUrl.find(L"http://") != 0 && targetUrl.find(L"https://") != 0) {
        targetUrl = L"https://" + targetUrl;
    }

    g_WebView->Navigate(targetUrl.c_str());
}

// Window process messaging router
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            // 1. Create the Address Input Bar (Text box)
            g_hWndUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"https://google.com",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0, 0, 0, 0, hWnd, (HMENU)IDC_URL_EDIT, hInst, NULL);

            // Set a cleaner default system font for the input bar
            SendMessage(g_hWndUrlEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

            // 2. Create the "Go" Activation Button
            g_hWndGoButton = CreateWindowExW(0, L"BUTTON", L"Go",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                0, 0, 0, 0, hWnd, (HMENU)IDC_GO_BUTTON, hInst, NULL);
            
            SendMessage(g_hWndGoButton, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
            break;
        }
        case WM_COMMAND: {
            // Trigger navigation if the user clicks the "Go" button
            if (LOWORD(wParam) == IDC_GO_BUTTON && HIWORD(wParam) == BN_CLICKED) {
                TriggerNavigation();
            }
            break;
        }
        case WM_SIZE:
            ResizeUI();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    
    // Load the baked icon resource instead of a standard arrow cursor icon
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = L"AVIS_FORM_BROWSER";
    RegisterClassExW(&wcex);

    // Update the window title text to Dark-Com-2
    g_hWndParent = CreateWindowExW(0, L"AVIS_FORM_BROWSER", L"Dark-Com-2", 
                                   WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                                   1200, 800, NULL, NULL, hInstance, NULL);

    if (!g_hWndParent) return 0;

    ShowWindow(g_hWndParent, nCmdShow);
    UpdateWindow(g_hWndParent);

    // DYNAMIC LOCAL CACHE ROUTING SYSTEM
    // Obtains the fully qualified local path of the running executable
    wchar_t exePathBuffer[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuffer, MAX_PATH);
    std::wstring modulePath(exePathBuffer);
    
    // Strip away the executable name to isolate the containing directory root
    size_t lastSlashPos = modulePath.find_last_of(L"\\/");
    std::wstring appRootDir = (lastSlashPos != std::wstring::npos) ? modulePath.substr(0, lastSlashPos) : L".";
    
    // Root the browser profile storage folder cleanly relative to the executable
    std::wstring cachePath = appRootDir + L"\\packages\\browser_cache";
    
    // Initialize WebView2 Engine with the dynamically isolated local cache path
    CreateCoreWebView2EnvironmentWithOptions(nullptr, cachePath.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) return result;

                env->CreateCoreWebView2Controller(g_hWndParent,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT res, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(res)) return res;

                            g_WebController = controller;
                            g_WebController->get_CoreWebView2(&g_WebView);

                            // Sync browser boundary layouts right away
                            ResizeUI();

                            // Sync URL edit block with the initialized target address
                            g_WebView->Navigate(L"https://mercwar01.byethost3.com");
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    // Standard Desktop execution loop tracking
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // Intercepts the Enter key when typing inside the URL text box so it triggers navigation
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetFocus() == g_hWndUrlEdit) {
            TriggerNavigation();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
