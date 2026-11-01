// ============================================================================
// File: dc2url.c
// Module: Dark-Com-2 Curl-Compatible Console Command Line Interface (dc2url v2.6)
// ============================================================================

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <wincrypt.h>

#define RESPONSE_BUFFER_SIZE (16 * 1024 * 1024) // 16MB buffer for large payloads

typedef int (*pfnRunHeadlessRequest)(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize);
typedef void (*pfnDarkCom_ShutdownEngine)(void);

// dc2ws2 function pointer typedefs
typedef BOOL (WINAPI *pfnDc2ws2Init)(void);
typedef void (WINAPI *pfnDc2ws2Cleanup)(void);
typedef HANDLE (WINAPI *pfnDc2ws2DownloadAsync)(const char *url, const char *dest_path, const void *config);

HMODULE LoadSystemDarkComHeadless(int forceLocalDir);

static void PrintUsage(void) {
    const char *const usageText = 
        "====================================================================\n"
        "  Dark-Com-2 Curl-Compatible Console Wrapper (dc2url v2.6)\n"
        "====================================================================\n"
        "Usage:    dc2url [options] <url>\n\n"
        "[PRIMARY CORE OPTIONS]\n"
        "  -X, --request <method>    Specify HTTP method (GET, POST, PUT, DELETE)\n"
        "  -d, --data <data>         Attach raw payload string or JSON body\n"
        "  -o, --output <file>       Stream raw response directly to disk file\n"
        "  -h, --help                Display this reference manual\n\n"
        "[SECONDARY PROTOCOL OPTIONS]\n"
        "  -H, --header <header>     Inject custom header (e.g., \"Authorization: Bearer...\")\n"
        "  -A, --user-agent <agent>  Override default client identification string\n"
        "  -s, --silent              Suppress all non-error output streams\n"
        "  -v, --verbose             Enable detailed diagnostic trace logs to stderr\n"
        "  -l, --local               Force loading DarkComHeadless.dll from executable folder\n"
        "  --nojs, -nojs             Bypass WebView2 JS engine (WinINet fast binary stub mode)\n"
        "  --nowv2, -nowv2           Bypass WebView2 environment (Force dynamic dc2ws2 fallback)\n"
        "  --dc2ws2, -dc2ws2         Route request via dc2ws2 high-performance network engine DLL\n"
        "  --noscript, -noscript     Direct raw download mode via dc2ws2 (bypasses JS / WebView2)\n"
        "====================================================================\n";
    printf("%s", usageText);
}

int main(int argc, char* argv[]) {
    _setmode(_fileno(stdout), _O_BINARY);

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    int useLocalDll = 0;
    int useDc2Ws2 = 0;
    
    // 1. Global Orchestration Pre-Scan Pass
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--local") == 0) {
            useLocalDll = 1;
        }
        if (strcmp(argv[i], "-dc2ws2") == 0 || strcmp(argv[i], "--dc2ws2") == 0 ||
            strcmp(argv[i], "-noscript") == 0 || strcmp(argv[i], "--noscript") == 0 ||
            strcmp(argv[i], "-nowv2") == 0 || strcmp(argv[i], "--nowv2") == 0) {
            useDc2Ws2 = 1;
        }
    }

    const char* rawTargetUrl = NULL;
    const char* method = "GET";
    const char* postData = NULL;
    const char* outputPath = NULL;
    char customHeader[512] = {0};
    char customUserAgent[256] = {0};
    int verbose = 0;
    int silent = 0;
    int useNoJs = 0;

    // 2. Bound-Checked Command Parameter Interception Pass
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) {
            if (i + 1 < argc) method = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) {
            if (i + 1 < argc) postData = argv[++i];
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) {
            if (i + 1 < argc) {
                snprintf(customHeader, sizeof(customHeader), "%s", argv[++i]);
            }
        } else if (strcmp(argv[i], "-A") == 0 || strcmp(argv[i], "--user-agent") == 0) {
            if (i + 1 < argc) {
                snprintf(customUserAgent, sizeof(customUserAgent), "%s", argv[++i]);
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) outputPath = argv[++i];
        } else if (strcmp(argv[i], "-I") == 0 || strcmp(argv[i], "--head") == 0) {
            method = "HEAD";
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            silent = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-nojs") == 0 || strcmp(argv[i], "--nojs") == 0 ||
                   strcmp(argv[i], "-nowv2") == 0 || strcmp(argv[i], "--nowv2") == 0) {
            useNoJs = 1;
        } else if (strcmp(argv[i], "-dc2ws2") == 0 || strcmp(argv[i], "--dc2ws2") == 0 ||
                   strcmp(argv[i], "-noscript") == 0 || strcmp(argv[i], "--noscript") == 0 ||
                   strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--local") == 0) {
            continue;
        } else if (argv[i][0] != '-') {
            rawTargetUrl = argv[i];
        }
    }

    if (!rawTargetUrl) {
        PrintUsage();
        return 1;
    }

    // --- ROUTE VECTOR 1: HIGH-PERFORMANCE NATIVE OVERRIDE ---
    if (useDc2Ws2) {
        char dc2ws2Path[MAX_PATH];
        GetModuleFileNameA(NULL, dc2ws2Path, MAX_PATH);
        char* lastSlash = strrchr(dc2ws2Path, '\\');
        if (lastSlash) *(lastSlash + 1) = '\0';
        strcat_s(dc2ws2Path, MAX_PATH, "dc2ws2.dll");

        HMODULE hWs2Dll = LoadLibraryA(dc2ws2Path);
        if (!hWs2Dll) hWs2Dll = LoadLibraryA("dc2ws2.dll");

        if (!hWs2Dll) {
            if (!silent) fprintf(stderr, "[ERROR] Could not resolve core engine dependency: dc2ws2.dll\n");
            return 1;
        }

        pfnDc2ws2Init Dc2ws2Init = (pfnDc2ws2Init)GetProcAddress(hWs2Dll, "Dc2ws2Init");
        pfnDc2ws2Cleanup Dc2ws2Cleanup = (pfnDc2ws2Cleanup)GetProcAddress(hWs2Dll, "Dc2ws2Cleanup");
        pfnDc2ws2DownloadAsync Dc2ws2DownloadAsync = (pfnDc2ws2DownloadAsync)GetProcAddress(hWs2Dll, "Dc2ws2DownloadAsync");

        if (!Dc2ws2Init || !Dc2ws2Cleanup || !Dc2ws2DownloadAsync) {
            if (!silent) fprintf(stderr, "[ERROR] Missing direct export addresses inside dc2ws2.dll.\n");
            FreeLibrary(hWs2Dll);
            return 1;
        }

        Dc2ws2Init();

        char tempOutputPath[MAX_PATH] = "";
        const char* finalOutputPath = outputPath;
        if (!finalOutputPath) {
            char tempDir[MAX_PATH];
            GetTempPathA(MAX_PATH, tempDir);
            GetTempFileNameA(tempDir, "dc2", 0, tempOutputPath);
            finalOutputPath = tempOutputPath;
        }

        HANDLE hThread = Dc2ws2DownloadAsync(rawTargetUrl, finalOutputPath, NULL);
        if (hThread && hThread != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);

            if (!outputPath) {
                FILE* f = fopen(finalOutputPath, "rb");
                if (f) {
                    char buf[8192];
                    size_t readBytes;
                    while ((readBytes = fread(buf, 1, sizeof(buf), f)) > 0) {
                        fwrite(buf, 1, readBytes, stdout);
                    }
                    fclose(f);
                }
                DeleteFileA(finalOutputPath);
            }
        } else {
            if (!silent) fprintf(stderr, "[ERROR] Background async network download thread failure.\n");
            Dc2ws2Cleanup();
            FreeLibrary(hWs2Dll);
            if (!outputPath) DeleteFileA(tempOutputPath);
            return 1;
        }

        Dc2ws2Cleanup();
        FreeLibrary(hWs2Dll);
        return 0;
    }

    // --- ROUTE VECTOR 2: STANDARD HEADLESS WEBVIEW2 ENGINE PIPELINE ---
    HMODULE hDll = LoadSystemDarkComHeadless(useLocalDll);
    if (!hDll) {
        fprintf(stderr, "[ERROR] Launcher could not resolve or load DarkComHeadless.dll.\n");
        return 1;
    }

    pfnRunHeadlessRequest RunHeadlessRequest = (pfnRunHeadlessRequest)GetProcAddress(hDll, "RunHeadlessRequest");
    pfnDarkCom_ShutdownEngine RunHeadlessShutdown = (pfnDarkCom_ShutdownEngine)GetProcAddress(hDll, "RunHeadlessShutdown");

    if (!RunHeadlessRequest) {
        fprintf(stderr, "[ERROR] Required procedural symbol 'RunHeadlessRequest' was missing inside library.\n");
        FreeLibrary(hDll);
        return 1;
    }

    if (customHeader[0] != '\0') SetEnvironmentVariableA("DC2_CUSTOM_HEADER", customHeader);
    if (customUserAgent[0] != '\0') SetEnvironmentVariableA("DC2_USER_AGENT", customUserAgent);
    if (useNoJs) {
        SetEnvironmentVariableA("DC2_NOJS", "1");
    } else {
        SetEnvironmentVariableA("DC2_NOJS", NULL);
    }

    // Signal download mode to headless module if output destination file is specified
    if (outputPath) {
        SetEnvironmentVariableA("DC2_DOWNLOAD_MODE", "1");
    } else {
        SetEnvironmentVariableA("DC2_DOWNLOAD_MODE", NULL);
    }

    char targetUrl[2300];
    snprintf(targetUrl, sizeof(targetUrl), "%s", rawTargetUrl);

    char* responseBuffer = (char*)malloc(RESPONSE_BUFFER_SIZE);
    if (!responseBuffer) {
        if (!silent) fprintf(stderr, "[ERROR] Out of memory allocation boundary constraints.\n");
        SetEnvironmentVariableA("DC2_CUSTOM_HEADER", NULL);
        SetEnvironmentVariableA("DC2_USER_AGENT", NULL);
        SetEnvironmentVariableA("DC2_NOJS", NULL);
        SetEnvironmentVariableA("DC2_DOWNLOAD_MODE", NULL);
        FreeLibrary(hDll);
        return 1;
    }
    memset(responseBuffer, 0, RESPONSE_BUFFER_SIZE);

    int bytesReceived = RunHeadlessRequest(targetUrl, method, postData, responseBuffer, RESPONSE_BUFFER_SIZE);

    SetEnvironmentVariableA("DC2_CUSTOM_HEADER", NULL);
    SetEnvironmentVariableA("DC2_USER_AGENT", NULL);
    SetEnvironmentVariableA("DC2_NOJS", NULL);
    SetEnvironmentVariableA("DC2_DOWNLOAD_MODE", NULL);

    if (bytesReceived < 0 || strncmp(responseBuffer, "ERROR:", 6) == 0) {
        if (!silent) {
            fprintf(stderr, "[ERROR] Remote Engine Exception: %s\n", 
                    (bytesReceived >= 0) ? responseBuffer + 6 : "Network connection aborted inside module.");
        }
        free(responseBuffer);
        if (RunHeadlessShutdown) RunHeadlessShutdown();
        FreeLibrary(hDll);
        return 1;
    }

    // --- OUTPUT / STREAM ROUTING & DECODING ---
    FILE* outDest = outputPath ? fopen(outputPath, "wb") : stdout;
    if (outDest) {
        fwrite(responseBuffer, 1, (size_t)bytesReceived, outDest);
        if (outputPath) fclose(outDest);
        else fflush(stdout);
    }

    free(responseBuffer);
    if (RunHeadlessShutdown) RunHeadlessShutdown();
    FreeLibrary(hDll);
    return 0;
}