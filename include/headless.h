#ifndef HEADLESS_H
#define HEADLESS_H

#include <windows.h>
#include <webview2.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

typedef struct {
    ICoreWebView2* webView;
    ICoreWebView2Controller* controller; // <-- Added to match headless.c expectations
    const wchar_t* url;
    const wchar_t* method;
    const wchar_t* postData;
    const wchar_t* contentType;
    int isVerbose;
} CommandLineArgs;

// Headless Execution Request Exposer
__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config);

// Runtime Usage Instructions Printer
void PrintUsageInstructions(void);

// Standard Argument Parser Logic
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

/**
 * @brief Master bridge mapping function that matches the signature expected by wmain.
 * Extracts the parsed data strings, loads them securely into static app storage buffers,
 * and passes the structure updates back into the runtime loop.
 */
BOOL ParseArgumentsCurl(int argc, wchar_t* argv[], CommandLineArgs* outArgs, 
                        wchar_t* postDataBuf, size_t postDataSize, 
                        wchar_t* headerBuf, size_t headerSize);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_H