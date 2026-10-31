// ============================================================================
// File: dc2url.c
// Module: Dark-Com-2 Curl-Compatible Console Command Line Interface (dc2url)
// ============================================================================
// Description:
//    Advanced CLI wrapper supporting custom headers, user-agents, head requests,
//    silent execution modes, and binary-safe file output streaming.
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

typedef int (*pfnDarkCom_ExecuteFetch)(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize);
typedef void (*pfnDarkCom_ShutdownEngine)(void);

HMODULE LoadSystemDarkComHeadless(int forceLocalDir);

static void PrintUsage(void) {
    const char *const usageText = 
        "====================================================================\n"
        "  Dark-Com-2 Curl-Compatible Console Wrapper (dc2url v2.3)\n"
        "====================================================================\n"
        "Usage:    dc2url [options] <url>\n\n"
        
        "[PRIMARY CORE OPTIONS]\n"
        "  -X, --request <method>    Specify HTTP method (GET, POST, PUT, DELETE)\n"
        "  -d, --data <data>         Attach raw payload string or JSON body\n"
        "  -o, --output <file>       Stream raw response directly to disk file\n"
        "  -h, --help                Display this multi-tier reference manual\n\n"
        
        "[SECONDARY PROTOCOL OPTIONS (Advanced Routing & Identity)]\n"
        "  -H, --header <header>     Inject custom header (e.g., \"Authorization: Bearer...\")\n"
        "  -A, --user-agent <agent>  Override default client identification string\n"
        "  -x, --proxy <url>         Route network transactions through a proxy server\n"
        "                            [Info: Specifies HTTP/HTTPS proxy tunnel endpoint]\n\n"
        
        "[MINOR SECONDARY OPTIONS (Diagnostics & Execution Behavior)]\n"
        "  -I, --head                Fetch response headers only (Shorthand for -X HEAD)\n"
        "  -k, --insecure            Allow insecure SSL connections and untrusted certs\n"
        "                            [Info: Bypasses cert revocation & root validation checks]\n"
        "  -s, --silent              Suppress all non-error output streams (pipeline safe)\n"
        "  -v, --verbose             Enable detailed diagnostic trace logs to stderr\n"
        "  -l, --local               Force loading DarkComHeadless.dll from executable folder\n"
        "  --nojs, -nojs             Bypass WebView2 JS engine (WinINet fast binary mode)\n"
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

    // Pre-scan arguments to catch the local dll switch early before loading module
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--local") == 0) {
            useLocalDll = 1;
            break;
        }
    }

    HMODULE hDll = LoadSystemDarkComHeadless(useLocalDll);
    if (!hDll) {
        fprintf(stderr, "[ERROR] System Launcher could not resolve or load DarkComHeadless.dll.\n");
        return 1;
    }

    pfnDarkCom_ExecuteFetch DarkCom_ExecuteFetch = (pfnDarkCom_ExecuteFetch)GetProcAddress(hDll, "DarkCom_ExecuteFetch");
    pfnDarkCom_ShutdownEngine RunHeadlessShutdown = (pfnDarkCom_ShutdownEngine)GetProcAddress(hDll, "RunHeadlessShutdown");

    if (!DarkCom_ExecuteFetch) {
        fprintf(stderr, "[ERROR] Required engine procedure 'DarkCom_ExecuteFetch' was not found.\n");
        FreeLibrary(hDll);
        return 1;
    }

    const char* rawTargetUrl = NULL;
    const char* method = "GET";
    const char* postData = NULL;
    const char* outputPath = NULL;
    char customHeader[512] = "";
    char customUserAgent[256] = "";
    int verbose = 0;
    int silent = 0;
    int useNoJs = 0;

    // Advanced Argument Parsing Loop
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
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--local") == 0) {
            // Handled in pre-scan
        } else if (strcmp(argv[i], "-nojs") == 0 || strcmp(argv[i], "--nojs") == 0) {
            useNoJs = 1;
        } else if (argv[i][0] != '-') {
            rawTargetUrl = argv[i];
        }
    }

    if (!rawTargetUrl) {
        PrintUsage();
        FreeLibrary(hDll);
        return 1;
    }

    // Set environment variables so DarkComHeadless.dll can read headers and user-agents
    if (customHeader[0] != '\0') {
        SetEnvironmentVariableA("DC2_CUSTOM_HEADER", customHeader);
    } else {
        SetEnvironmentVariableA("DC2_CUSTOM_HEADER", NULL);
    }

    if (customUserAgent[0] != '\0') {
        SetEnvironmentVariableA("DC2_USER_AGENT", customUserAgent);
    } else {
        SetEnvironmentVariableA("DC2_USER_AGENT", NULL);
    }

    char targetUrl[2300];
    if (useNoJs && strstr(rawTargetUrl, "-nojs") == NULL) {
        snprintf(targetUrl, sizeof(targetUrl), "%s -nojs", rawTargetUrl);
    } else {
        snprintf(targetUrl, sizeof(targetUrl), "%s", rawTargetUrl);
    }

    if (verbose && !silent) {
        fprintf(stderr, "[VERBOSE] Target Destination URL : %s\n", targetUrl);
        fprintf(stderr, "[VERBOSE] Request Method         : %s\n", method);
        if (customHeader[0]) fprintf(stderr, "[VERBOSE] Custom Header          : %s\n", customHeader);
        if (customUserAgent[0]) fprintf(stderr, "[VERBOSE] Custom User-Agent     : %s\n", customUserAgent);
        fprintf(stderr, "[VERBOSE] Execution Engine       : %s\n", useNoJs ? "Fast-Path WinINet Bypass (-nojs)" : "Stateful Microsoft Edge WebView2");
        fprintf(stderr, "[VERBOSE] DLL Source Mode        : %s\n", useLocalDll ? "Forced Local Executable Directory" : "Standard Registry / Automatic Resolution");
    }

    char* responseBuffer = (char*)malloc(RESPONSE_BUFFER_SIZE);
    if (!responseBuffer) {
        if (!silent) fprintf(stderr, "[ERROR] Memory allocation failed for response buffer.\n");
        SetEnvironmentVariableA("DC2_CUSTOM_HEADER", NULL);
        SetEnvironmentVariableA("DC2_USER_AGENT", NULL);
        FreeLibrary(hDll);
        return 1;
    }
    memset(responseBuffer, 0, RESPONSE_BUFFER_SIZE);

    int bytesReceived = DarkCom_ExecuteFetch(targetUrl, method, postData, responseBuffer, RESPONSE_BUFFER_SIZE);

    // Clear environment variables immediately after fetch
    SetEnvironmentVariableA("DC2_CUSTOM_HEADER", NULL);
    SetEnvironmentVariableA("DC2_USER_AGENT", NULL);

    if (bytesReceived < 0) {
        if (!silent) fprintf(stderr, "[ERROR] Network fetch execution failed inside DLL engine.\n");
        free(responseBuffer);
        if (RunHeadlessShutdown) RunHeadlessShutdown();
        FreeLibrary(hDll);
        return 1;
    }

    if (strncmp(responseBuffer, "ERROR:", 6) == 0) {
        if (!silent) fprintf(stderr, "[ERROR] Remote Exception Encountered: %s\n", responseBuffer + 6);
        free(responseBuffer);
        if (RunHeadlessShutdown) RunHeadlessShutdown();
        FreeLibrary(hDll);
        return 1;
    }

    DWORD decodedLen = 0;
    BYTE* decodedBytes = NULL;

    if (!useNoJs && CryptStringToBinaryA(responseBuffer, 0, CRYPT_STRING_BASE64, NULL, &decodedLen, NULL, NULL)) {
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
            char* writePtr = responseBuffer;
            DWORD writeLen = (DWORD)bytesReceived;

            // If -nojs fast-path was used, strip HTTP headers (\r\n\r\n) before writing binary/text
            if (useNoJs) {
                char* body = strstr(responseBuffer, "\r\n\r\n");
                if (body) {
                    body += 4;
                    writeLen = (DWORD)(bytesReceived - (body - responseBuffer));
                    writePtr = body;
                }
            }

            if (decodedBytes && decodedLen > 0 && !useNoJs) {
                fwrite(decodedBytes, 1, decodedLen, f);
            } else {
                fwrite(writePtr, 1, writeLen, f);
            }
            fclose(f);
            if (verbose && !silent) fprintf(stderr, "[VERBOSE] File successfully written to disk: %s (%d bytes)\n", outputPath, (int)writeLen);
        } else {
            if (!silent) fprintf(stderr, "[ERROR] Could not open destination file path for writing: '%s'\n", outputPath);
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