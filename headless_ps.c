/* ============================================================================
   headless_ps.c - CLI Wrapper Host Executable (Pure C Version)
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

#ifndef CLSCTX_INPROC_SERVER
#define CLSCTX_INPROC_SERVER 0x1
#endif

// Define WebView2 Environment Options CLSID manually for pure C compatibility
// {2FDE08A8-1E9A-4566-8C05-9FEA9E66CEDD}
static const GUID CDECL Custom_CLSID_CoreWebView2EnvironmentOptions = { 0x2fde08a8, 0x1e9a, 0x4566, { 0x8c, 0x05, 0x9f, 0xea, 0x9e, 0x66, 0xce, 0xdd } };

typedef const char* (__cdecl *RunHeadlessRequest_t)(const CommandLineArgs*);

static ICoreWebView2* g_WebView = NULL;
static ICoreWebView2Controller* g_WebController = NULL;
static volatile LONG g_IsInitComplete = 0;

// Generic QueryInterface / AddRef / Release implementation for custom handlers
static HRESULT STDMETHODCALLTYPE Handler_QueryInterface(void* This, REFIID riid, void** ppvObject) {
    if (!ppvObject) return E_POINTER;
    *ppvObject = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
        *ppvObject = This;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Handler_AddRef(void* This) { return 1; }
static ULONG STDMETHODCALLTYPE Handler_Release(void* This) { return 1; }

// Forward declarations of handler invoke methods
static HRESULT STDMETHODCALLTYPE ControllerCompleted_Invoke(
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This,
    HRESULT errorCode,
    ICoreWebView2Controller* createdController);

static HRESULT STDMETHODCALLTYPE EnvironmentCompleted_Invoke(
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This,
    HRESULT errorCode,
    ICoreWebView2Environment* createdEnvironment);

// Handler Vtbl and Object definitions
typedef struct {
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl;
} ControllerHandler;

typedef struct {
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl* lpVtbl;
    HWND hwndParent;
} EnvironmentHandler;

static HRESULT STDMETHODCALLTYPE ControllerCompleted_Invoke(
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* This,
    HRESULT errorCode,
    ICoreWebView2Controller* createdController)
{
    if (SUCCEEDED(errorCode) && createdController) {
        g_WebController = createdController;
        ICoreWebView2Controller_AddRef(g_WebController);
        ICoreWebView2Controller_get_CoreWebView2(g_WebController, &g_WebView);
    }
    InterlockedExchange(&g_IsInitComplete, 1);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE EnvironmentCompleted_Invoke(
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* This,
    HRESULT errorCode,
    ICoreWebView2Environment* createdEnvironment)
{
    EnvironmentHandler* envHandler = (EnvironmentHandler*)This;
    
    if (SUCCEEDED(errorCode) && createdEnvironment) {
        static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctrlVtbl = {
            (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*, REFIID, void**))Handler_QueryInterface,
            (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*))Handler_AddRef,
            (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*))Handler_Release,
            ControllerCompleted_Invoke
        };
        static ControllerHandler ctrlObj;
        ctrlObj.lpVtbl = &ctrlVtbl;

        ICoreWebView2Environment_CreateCoreWebView2Controller(
            createdEnvironment, 
            envHandler->hwndParent, 
            (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)&ctrlObj
        );
    } else {
        InterlockedExchange(&g_IsInitComplete, 1);
    }
    return S_OK;
}

static BOOL InitializeHeadlessWebView(void) {
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, DefWindowProcW, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"DarkComHeadlessHost", NULL };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"DarkComHeadlessHost", L"DarkCom Headless Broker", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) return FALSE;

    static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl envVtbl = {
        (HRESULT(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*, REFIID, void**))Handler_QueryInterface,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*))Handler_AddRef,
        (ULONG(STDMETHODCALLTYPE*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*))Handler_Release,
        EnvironmentCompleted_Invoke
    };
    static EnvironmentHandler envObj;
    envObj.lpVtbl = &envVtbl;
    envObj.hwndParent = hwnd;

    // Build persistent local profile directory path next to executable for session/cookie persistence
    wchar_t profilePath[MAX_PATH];
    GetModuleFileNameW(NULL, profilePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(profilePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    wcscat_s(profilePath, MAX_PATH, L"\\darkcom_browser_profile");

    // Initialize environment options with security bypass switches for localhost & remote requests
    ICoreWebView2EnvironmentOptions* options = NULL;
    HRESULT hr = CoCreateInstance(
        &Custom_CLSID_CoreWebView2EnvironmentOptions, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        &IID_ICoreWebView2EnvironmentOptions, 
        (void**)&options
    );

    if (SUCCEEDED(hr) && options) {
        // Disable web security and allow insecure content to support localhost, local IPs, and remote hosts seamlessly
        ICoreWebView2EnvironmentOptions_put_AdditionalBrowserArguments(options, L"--disable-web-security --allow-running-insecure-content");
    }

    hr = CreateCoreWebView2EnvironmentWithOptions(NULL, profilePath, options, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)&envObj);
    
    if (options) {
        ICoreWebView2EnvironmentOptions_Release(options);
    }

    if (FAILED(hr)) return FALSE;

    while (!InterlockedOr(&g_IsInitComplete, 0)) {
        MsgWaitForMultipleObjectsEx(0, NULL, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (g_WebView != NULL);
}

void PrintUsageInstructions(void) {
    wprintf(L"\n============================================================\n");
    wprintf(L" Dark-Com-2 Curl-Compatible Console Wrapper (darkcom2)        \n");
    wprintf(L"============================================================\n");
    wprintf(L"Usage:   darkcom2.exe [options] <url>\n\n");
    wprintf(L"Options:\n");
    wprintf(L"  -X, --request <method>     Specify request method (GET, POST)\n");
    wprintf(L"  -d, --data <data>          Pass payload string for requests\n");
    wprintf(L"  -H, --header <header>      Pass custom HTTP headers (e.g. Content-Type)\n");
    wprintf(L"  -v, --verbose              Enable verbose internal process logs\n");
    wprintf(L"============================================================\n");
}

BOOL ParseArgumentsCurl(int argc, wchar_t* argv[], CommandLineArgs* outArgs, wchar_t* postDataBuf, size_t postDataBufLen, wchar_t* headerBuf, size_t headerBufLen) {
    outArgs->webView = NULL;
    outArgs->url = NULL;
    outArgs->method = L"GET";
    outArgs->postData = NULL;
    outArgs->contentType = NULL;
    outArgs->isVerbose = 0;

    if (argc < 2) return FALSE;

    postDataBuf[0] = L'\0';
    headerBuf[0] = L'\0';

    for (int i = 1; i < argc; i++) {
        wchar_t* arg = argv[i];

        if (_wcsicmp(arg, L"-X") == 0 || _wcsicmp(arg, L"--request") == 0) {
            if (i + 1 < argc) outArgs->method = argv[++i];
        } 
        else if (_wcsicmp(arg, L"-v") == 0 || _wcsicmp(arg, L"--verbose") == 0) {
            outArgs->isVerbose = 1;
        } 
        else if (_wcsicmp(arg, L"-H") == 0 || _wcsicmp(arg, L"--header") == 0) {
            if (i + 1 < argc) {
                wchar_t* headerVal = argv[++i];
                if (_wcsnicmp(headerVal, L"Content-Type:", 13) == 0) {
                    wchar_t* val = headerVal + 13;
                    while (*val == L' ') val++;
                    wcsncpy_s(headerBuf, headerBufLen, val, _TRUNCATE);
                    outArgs->contentType = headerBuf;
                }
            }
        }
        else if (_wcsicmp(arg, L"-d") == 0 || _wcsicmp(arg, L"--data") == 0 || _wcsicmp(arg, L"--data-raw") == 0) {
            if (i + 1 < argc) {
                wcsncpy_s(postDataBuf, postDataBufLen, argv[++i], _TRUNCATE);
                outArgs->postData = postDataBuf;
                if (_wcsicmp(outArgs->method, L"GET") == 0) {
                    outArgs->method = L"POST";
                }
            }
        } 
        else if (arg[0] != L'-') {
            if (!outArgs->url) outArgs->url = arg;
        }
    }
    return (outArgs->url != NULL);
}

int wmain(int argc, wchar_t* argv[]) {
    CommandLineArgs config;
    static wchar_t postDataStorage[8192] = {0};
    static wchar_t headerStorage[4096] = {0};

    SetConsoleOutputCP(CP_UTF8);

    if (!ParseArgumentsCurl(argc, argv, &config, postDataStorage, 8192, headerStorage, 4096)) {
        PrintUsageInstructions();
        return 1;
    }

    if (config.isVerbose) {
        wprintf(L"[INFO] Initializing Core Layer for Target: %s\n", config.url);
    }

    if (_wcsicmp(config.method, L"POST") == 0 && (config.postData == NULL || wcslen(config.postData) == 0)) {
        config.postData = L"";
    }

    HMODULE hEngineDll = LoadLibraryW(L"DarkComHeadless.dll");
    if (!hEngineDll) {
        fwprintf(stderr, L"[FATAL ERROR] Unable to resolve 'DarkComHeadless.dll'. Ensure binary outputs exist in application path.\n");
        return 2;
    }

    RunHeadlessRequest_t RunHeadlessRequest = (RunHeadlessRequest_t)GetProcAddress(hEngineDll, "RunHeadlessRequest");
    if (!RunHeadlessRequest) {
        fwprintf(stderr, L"[FATAL ERROR] Linker error: Failed to map 'RunHeadlessRequest' function pointer from target DLL.\n");
        FreeLibrary(hEngineDll);
        return 3;
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (config.isVerbose) {
        wprintf(L"[INFO] Spawning windowless broker instance. Initializing WebView2 environment...\n");
    }

    if (!InitializeHeadlessWebView()) {
        fwprintf(stderr, L"[FATAL ERROR] Failed to initialize WebView2 headless browser runtime.\n");
        CoUninitialize();
        FreeLibrary(hEngineDll);
        return 4;
    }

    config.webView = g_WebView;

    if (config.isVerbose) {
        char targetUrlUtf8[2048] = {0};
        char methodUtf8[32] = {0};
        WideCharToMultiByte(CP_UTF8, 0, config.url, -1, targetUrlUtf8, sizeof(targetUrlUtf8) - 1, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, config.method, -1, methodUtf8, sizeof(methodUtf8) - 1, NULL, NULL);
        
        printf("> %s %s HTTP/1.1\n", methodUtf8, targetUrlUtf8);
        printf("> Host: HeadlessBroker\n");
        printf("> User-Agent: DarkCom2/HeadlessEngine\n");
        printf("> Accept: */*\n");
        if (config.contentType) {
            char ctypeUtf8[256] = {0};
            WideCharToMultiByte(CP_UTF8, 0, config.contentType, -1, ctypeUtf8, sizeof(ctypeUtf8) - 1, NULL, NULL);
            printf("> Content-Type: %s\n", ctypeUtf8);
        }
        printf(">\n");
    }

    wprintf(L"[Uplinking] Spawning DarkCom2 Request...\n");
    const char* serverResponseString = RunHeadlessRequest(&config);
    wprintf(L"[Downlinking] Processing Response...\n");

    if (serverResponseString != NULL) {
        const char* statusPtr = strstr(serverResponseString, "\"status\":");
        const char* bodyPtr = strstr(serverResponseString, ",\"body\":\"");

        if (config.isVerbose && statusPtr && bodyPtr) {
            int statusCode = 0;
            sscanf_s(statusPtr, "\"status\":%d", &statusCode);
            
            printf("< HTTP/1.1 %d OK\n", statusCode ? statusCode : 200);
            printf("< Server: DarkComHeadlessBroker\n");
            
            const char* headersStart = strstr(serverResponseString, "\"headers\":{");
            if (headersStart) {
                headersStart += 11;
                const char* headersEnd = strchr(headersStart, '}');
                if (headersEnd && headersEnd > headersStart) {
                    size_t len = headersEnd - headersStart;
                    char* tempHdr = (char*)malloc(len + 1);
                    if (tempHdr) {
                        memcpy(tempHdr, headersStart, len);
                        tempHdr[len] = '\0';
                        
                        char* token = NULL;
                        char* nextToken = NULL;
                        token = strtok_s(tempHdr, ",", &nextToken);
                        while (token != NULL) {
                            printf("< %s\n", token);
                            token = strtok_s(NULL, ",", &nextToken);
                        }
                        free(tempHdr);
                    }
                }
            }
            printf("<\n");
            
            bodyPtr += 9;
            size_t bodyLen = strlen(bodyPtr);
            if (bodyLen > 2) {
                char* finalOutput = _strdup(bodyPtr);
                if (finalOutput) {
                    if (finalOutput[bodyLen - 1] == '}') finalOutput[bodyLen - 1] = '\0';
                    if (finalOutput[bodyLen - 2] == '"') finalOutput[bodyLen - 2] = '\0';
                    printf("%s\n", finalOutput);
                    free(finalOutput);
                }
            }
        } else {
            printf("%s\n", serverResponseString);
        }
    } else {
        fwprintf(stderr, L"[ERROR] Network transaction returned empty or null data tracking frames.\n");
    }

    if (g_WebController) {
        ICoreWebView2Controller_Close(g_WebController);
        ICoreWebView2Controller_Release(g_WebController);
    }
    if (g_WebView) {
        ICoreWebView2_Release(g_WebView);
    }

    CoUninitialize();
    FreeLibrary(hEngineDll);
    return 0;
}