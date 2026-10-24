#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headless_ps.h"

// Define the function pointer signature to match your compiled DarkComHeadless interface
typedef const char* (*RunHeadlessRequest_t)(const wchar_t*, const wchar_t*);

void PrintUsageInstructions(void) {
    wprintf(L"\n============================================================\n");
    wprintf(L" Dark-Com-2 PowerShell Console Wrapper Framework (dcurl) \n");
    wprintf(L"============================================================\n");
    wprintf(L"Usage:    darkcom2.exe <url> [-X <method>] [-d <data>] [-v]\n\n");
    wprintf(L"Options:\n");
    wprintf(L"  <url>         The target URI endpoint (e.g., https://example.com)\n");
    wprintf(L"  -X <method>   Specify request command method (GET or POST)\n");
    wprintf(L"  -d <data>     Pass URL-encoded string parameters for POST payloads\n");
    wprintf(L"  -v            Enable verbose internal process state tracking logs\n\n");
    wprintf(L"Example:\n");
    wprintf(L"  darkcom2.exe https://httpbin.org -X POST -d \"foo=bar&baz=qux\"\n");
    wprintf(L"============================================================\n");
}

BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs) {
    // Zero out the allocation states
    outArgs->url = NULL;
    outArgs->method = L"GET"; // Default behavior
    outArgs->postData = NULL;
    outArgs->isVerbose = 0;

    if (argc < 2) return FALSE;

    // The first parameter is implicitly assigned as the master URL target
    outArgs->url = argv[1];

    // Iterative loop processing to extract flags passed down from the console host
    for (int i = 2; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-X") == 0 && (i + 1 < argc)) {
            outArgs->method = argv[++i];
        } else if (_wcsicmp(argv[i], L"-d") == 0 && (i + 1 < argc)) {
            outArgs->postData = argv[++i];
            // Automatically switch method fallback to POST if data payload is explicitly supplied
            if (_wcsicmp(outArgs->method, L"GET") == 0) {
                outArgs->method = L"POST";
            }
        } else if (_wcsicmp(argv[i], L"-v") == 0) {
            outArgs->isVerbose = 1;
        }
    }

    return (outArgs->url != NULL);
}

int wmain(int argc, wchar_t* argv[]) {
    CommandLineArgs config;

    // Ensure wide character console translations process UTF-8 string streams properly
    SetConsoleOutputCP(CP_UTF8);

    if (!ParsePowerShellArguments(argc, argv, &config)) {
        PrintUsageInstructions();
        return 1;
    }

    if (config.isVerbose) {
        wprintf(L"[INFO] Initializing Core Layer for Target: %s\n", config.url);
        wprintf(L"[INFO] Request Method Payload Specified: %s\n", config.method);
    }

    // Force validation synchronization matching method payloads
    if (_wcsicmp(config.method, L"POST") == 0 && config.postData == NULL) {
        config.postData = L""; // Provide clean initialization mapping to avoid null pointers
    }

    // 1. Dynamically connect to the underlying Dark-Com-2 headless engine library
    HMODULE hEngineDll = LoadLibrary(L"DarkComHeadless.dll");
    if (!hEngineDll) {
        fwprintf(stderr, L"[FATAL ERROR] Unable to resolve 'DarkComHeadless.dll'. Ensure binary outputs exist in application path.\n");
        return 2;
    }

    // 2. Resolve the function pointer address mapping
    RunHeadlessRequest_t RunHeadlessRequest = (RunHeadlessRequest_t)GetProcAddress(hEngineDll, "RunHeadlessRequest");
    if (!RunHeadlessRequest) {
        fwprintf(stderr, L"[FATAL ERROR] Linker error: Failed to map 'RunHeadlessRequest' function pointer from target DLL.\n");
        FreeLibrary(hEngineDll);
        return 3;
    }

    // 3. Initialize OLE Apartment model required for headless COM processing loops
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (config.isVerbose) {
        wprintf(L"[INFO] Spawning windowless broker instance. Executing background transaction...\n");
    }

    // 4. Fire the headless WebView2 processing transaction routine directly
    const char* serverResponseString = RunHeadlessRequest(config.url, config.postData);

    // 5. Output raw results back down into the caller terminal environment pipeline stream
    if (serverResponseString != NULL) {
        // Prints clean string response data (e.g. PHP output, raw JSON, plain text) directly to stdout
        printf("%s\n", serverResponseString);
    } else {
        fwprintf(stderr, L"[ERROR] Network transaction returned empty or null data tracking frames.\n");
    }

    // 6. Tear down execution footprints cleanly
    CoUninitialize();
    FreeLibrary(hEngineDll);

    return 0;
}
