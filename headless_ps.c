// ============================================================================
// dc2url.c - Curl-Compatible CLI Wrapper for DarkComHeadless Engine (Optimized)
// ============================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <wincrypt.h>

#define RESPONSE_BUFFER_SIZE (16 * 1024 * 1024) // Increased to 16MB to support larger payloads

typedef int (*pfnDarkCom_ExecuteFetch)(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize);
typedef void (*pfnDarkCom_ShutdownEngine)(void);

HMODULE LoadSystemDarkComHeadless(void);

static void PrintUsage(void) {
    printf("============================================================\n");
    printf("  Dark-Com-2 Curl-Compatible Console Wrapper (dc2url)\n");
    printf("============================================================\n");
    printf("Usage:    dc2url [options] <url>\n\n");
    printf("Options:\n");
    printf("  -X, --request <method>    Specify request method (GET, POST)\n");
    printf("  -d, --data <data>         Pass payload string for requests\n");
    printf("  -o, --output <file>       Write raw response body directly to file\n");
    printf("  -v, --verbose             Enable verbose logs\n");
    printf("============================================================\n");
}

int main(int argc, char* argv[]) {
    _setmode(_fileno(stdout), _O_BINARY);

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    HMODULE hDll = LoadSystemDarkComHeadless();
    if (!hDll) {
        fprintf(stderr, "[-] Fatal Error: Failed to resolve or load DarkComHeadless.dll from system path.\n");
        return 1;
    }

    pfnDarkCom_ExecuteFetch DarkCom_ExecuteFetch = (pfnDarkCom_ExecuteFetch)GetProcAddress(hDll, "DarkCom_ExecuteFetch");
    pfnDarkCom_ShutdownEngine RunHeadlessShutdown = (pfnDarkCom_ShutdownEngine)GetProcAddress(hDll, "RunHeadlessShutdown");

    if (!DarkCom_ExecuteFetch) {
        fprintf(stderr, "[-] Fatal Error: Export 'DarkCom_ExecuteFetch' not found in DLL.\n");
        FreeLibrary(hDll);
        return 1;
    }

    const char* targetUrl = NULL;
    const char* method = "GET";
    const char* postData = NULL;
    const char* outputPath = NULL;
    int verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "--request") == 0) {
            if (i + 1 < argc) method = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) {
            if (i + 1 < argc) postData = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) outputPath = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (argv[i][0] != '-') {
            targetUrl = argv[i];
        }
    }

    if (!targetUrl) {
        PrintUsage();
        FreeLibrary(hDll);
        return 1;
    }

    if (verbose) {
        fprintf(stderr, "[VERBOSE] Target URL: %s\n", targetUrl);
        fprintf(stderr, "[VERBOSE] Method: %s\n", method);
        fprintf(stderr, "[VERBOSE] Executing request via DarkComHeadless DLL...\n");
    }

    char* responseBuffer = (char*)malloc(RESPONSE_BUFFER_SIZE);
    if (!responseBuffer) {
        fprintf(stderr, "[-] Fatal Error: Failed to allocate memory for response buffer.\n");
        FreeLibrary(hDll);
        return 1;
    }
    responseBuffer[0] = '\0';

    int success = DarkCom_ExecuteFetch(targetUrl, method, postData, responseBuffer, RESPONSE_BUFFER_SIZE);
    if (!success) {
        fprintf(stderr, "[-] Fetch execution failed inside DLL engine.\n");
        free(responseBuffer);
        if (RunHeadlessShutdown) RunHeadlessShutdown();
        FreeLibrary(hDll);
        return 1;
    }

    // Check if the engine returned a JavaScript exception string
    if (strncmp(responseBuffer, "ERROR:", 6) == 0) {
        fprintf(stderr, "[-] JavaScript Fetch Error: %s\n", responseBuffer + 6);
        free(responseBuffer);
        if (RunHeadlessShutdown) RunHeadlessShutdown();
        FreeLibrary(hDll);
        return 1;
    }

    DWORD decodedLen = 0;
    BYTE* decodedBytes = NULL;

    // Decode base64 payload returned from WebView2
    if (CryptStringToBinaryA(responseBuffer, 0, CRYPT_STRING_BASE64, NULL, &decodedLen, NULL, NULL)) {
        decodedBytes = (BYTE*)malloc(decodedLen);
        if (decodedBytes) {
            if (!CryptStringToBinaryA(responseBuffer, 0, CRYPT_STRING_BASE64, decodedBytes, &decodedLen, NULL, NULL)) {
                free(decodedBytes);
                decodedBytes = NULL;
            }
        }
    }

    if (outputPath) {
        FILE* f = fopen(outputPath, "wb");
        if (f) {
            if (decodedBytes) {
                fwrite(decodedBytes, 1, decodedLen, f);
            } else {
                fwrite(responseBuffer, 1, strlen(responseBuffer), f);
            }
            fclose(f);
            if (verbose) fprintf(stderr, "[VERBOSE] Output written successfully to: %s\n", outputPath);
        } else {
            fprintf(stderr, "[-] Error: Could not open output file '%s' for writing.\n", outputPath);
        }
    } else {
        if (decodedBytes) {
            fwrite(decodedBytes, 1, decodedLen, stdout);
        } else {
            printf("%s", responseBuffer);
        }
    }

    if (decodedBytes) free(decodedBytes);
    free(responseBuffer);

    if (RunHeadlessShutdown) {
        RunHeadlessShutdown();
    }

    FreeLibrary(hDll);
    return 0;
}