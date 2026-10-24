/* ============================================================================
   main.c - CLI Wrapper Host Executable - Precise Token Edition
   ============================================================================ */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headless.h"

typedef const char* (__cdecl *RunHeadlessRequest_t)(const CommandLineArgs*);

void PrintUsageInstructions(void) {
    wprintf(L"\n============================================================\n");
    wprintf(L" Dark-Com-2 PowerShell Console Wrapper Framework (dcurl) \n");
    wprintf(L"============================================================\n");
    wprintf(L"Usage:    darkcom2.exe <url> [-X <method>] [-d <data>] [-v]\n\n");
    wprintf(L"Options:\n");
    wprintf(L"  <url>         The target URI endpoint (e.g., https://example.com)\n");
    wprintf(L"  -X <method>   Specify request command method (GET or POST)\n");
    wprintf(L"  -d <data>     Pass JSON payload string for POST requests\n");
    wprintf(L"  -v            Enable verbose internal process state tracking logs\n");
    wprintf(L"============================================================\n");
}

BOOL ParseArgumentsPrecise(int argc, wchar_t* argv[], CommandLineArgs* outArgs, wchar_t* postDataBuf, size_t postDataBufLen) {
    outArgs->url = NULL;
    outArgs->method = L"GET";
    outArgs->postData = NULL;
    outArgs->isVerbose = 0;

    if (argc < 2) return FALSE;

    outArgs->url = argv[1];

    for (int i = 2; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-X") == 0 && (i + 1 < argc)) {
            outArgs->method = argv[++i];
        } else if (_wcsicmp(argv[i], L"-v") == 0) {
            outArgs->isVerbose = 1;
        } else if (_wcsicmp(argv[i], L"-d") == 0 && (i + 1 < argc)) {
            wcsncpy_s(postDataBuf, postDataBufLen, argv[i + 1], _TRUNCATE);
            outArgs->postData = postDataBuf;
            if (_wcsicmp(outArgs->method, L"GET") == 0) {
                outArgs->method = L"POST";
            }
            i++; // skip the payload argument value
        }
    }

    return (outArgs->url != NULL);
}

int wmain(int argc, wchar_t* argv[]) {
    CommandLineArgs config;
    wchar_t postDataStorage[4096] = {0};

    SetConsoleOutputCP(CP_UTF8);

    if (!ParseArgumentsPrecise(argc, argv, &config, postDataStorage, 4096)) {
        PrintUsageInstructions();
        return 1;
    }

    if (config.isVerbose) {
        wprintf(L"[INFO] Initializing Core Layer for Target: %s\n", config.url);
    }

    if (_wcsicmp(config.method, L"POST") == 0 && (config.postData == NULL || wcslen(config.postData) == 0)) {
        config.postData = L"";
    }

    HMODULE hEngineDll = LoadLibrary(L"DarkComHeadless.dll");
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
        wprintf(L"[INFO] Spawning windowless broker instance. Executing background transaction...\n");
    }

    wprintf(L"[Uplinking] Dcom...\n");

    const char* serverResponseString = RunHeadlessRequest(&config);

    wprintf(L"[Downlinking] Spawning Dcom...\n");

    if (serverResponseString != NULL) {
        printf("%s\n", serverResponseString);
    } else {
        fwprintf(stderr, L"[ERROR] Network transaction returned empty or null data tracking frames.\n");
    }

    CoUninitialize();
    FreeLibrary(hEngineDll);

    return 0;
}