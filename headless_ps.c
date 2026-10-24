/* ============================================================================
   main.c - CLI Wrapper Host Executable - Auto-Quoting JSON Edition
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
    wprintf(L"Usage:   darkcom2.exe <url> [-X <method>] [-d <data>] [-v]\n\n");
    wprintf(L"Options:\n");
    wprintf(L"  <url>         The target URI endpoint (e.g., https://example.com)\n");
    wprintf(L"  -X <method>   Specify request command method (GET or POST)\n");
    wprintf(L"  -d <data>     Pass JSON payload string for POST requests\n");
    wprintf(L"  -v            Enable verbose internal process state tracking logs\n");
    wprintf(L"============================================================\n");
}

// Helper to sanitize/auto-wrap raw key-value pairs into valid JSON if quotes were stripped
void NormalizeJsonPayload(const wchar_t* src, wchar_t* dest, size_t destSize) {
    // If it already looks like a properly structured JSON or has quotes, preserve it
    size_t len = wcslen(src);
    if (len == 0) {
        wcscpy_s(dest, destSize, L"{}");
        return;
    }

    // Check if it's already wrapped in braces
    if (src[0] == L'{' && src[len - 1] == L'}') {
        // Check if inner keys lack quotes (e.g., {pretty:print})
        // Simple heuristic: if there's a colon but no double quotes around the key before it
        const wchar_t* colon = wcschr(src, L':');
        if (colon && wcschr(src, L'"') == NULL) {
            // Extract key and value between { and }
            wchar_t inner[4096] = {0};
            wcsncpy_s(inner, 4096, src + 1, len - 2);
            
            wchar_t* context = NULL;
            wchar_t* key = wcstok_s(inner, L":", &context);
            wchar_t* val = wcstok_s(NULL, L":", &context);

            if (key && val) {
                // Trim leading/trailing spaces
                while (*key == L' ') key++;
                size_t kLen = wcslen(key);
                while (kLen > 0 && key[kLen - 1] == L' ') key[--kLen] = L'\0';

                while (*val == L' ') val++;
                size_t vLen = wcslen(val);
                while (vLen > 0 && val[vLen - 1] == L' ') val[--vLen] = L'\0';

                // Format into proper valid JSON with quotes
                swprintf_s(dest, destSize, L"{\"%s\": \"%s\"}", key, val);
                return;
            }
        }
    }

    // Fallback: copy as-is if normalization isn't triggered
    wcscpy_s(dest, destSize, src);
}

BOOL ParseArgumentsPrecise(int argc, wchar_t* argv[], CommandLineArgs* outArgs, wchar_t* postDataBuf, size_t postDataBufLen) {
    outArgs->url = NULL;
    outArgs->method = L"GET";
    outArgs->postData = NULL;
    outArgs->isVerbose = 0;

    if (argc < 2) return FALSE;

    outArgs->url = argv[1];

    wchar_t rawPostData[4096] = {0};

    for (int i = 2; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-X") == 0 && (i + 1 < argc)) {
            outArgs->method = argv[++i];
        } else if (_wcsicmp(argv[i], L"-v") == 0) {
            outArgs->isVerbose = 1;
        } else if (_wcsicmp(argv[i], L"-d") == 0 && (i + 1 < argc)) {
            size_t currentLen = 0;
            rawPostData[0] = L'\0';

            i++; // Move to payload start
            while (i < argc) {
                if (argv[i][0] == L'-' && wcslen(argv[i]) == 2 && (argv[i][1] == L'X' || argv[i][1] == L'v' || argv[i][1] == L'd')) {
                    i--; 
                    break;
                }

                size_t tokenLen = wcslen(argv[i]);
                if (currentLen + tokenLen + 2 < 4096) {
                    if (currentLen > 0) {
                        wcscat_s(rawPostData, 4096, L" ");
                        currentLen++;
                    }
                    wcscat_s(rawPostData, 4096, argv[i]);
                    currentLen += tokenLen;
                }
                i++;
            }

            // Automatically restore missing JSON quotes stripped by PowerShell
            NormalizeJsonPayload(rawPostData, postDataBuf, postDataBufLen);

            outArgs->postData = postDataBuf;
            if (_wcsicmp(outArgs->method, L"GET") == 0) {
                outArgs->method = L"POST";
            }
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