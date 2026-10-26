#ifndef HEADLESS_H
#define HEADLESS_H

#include <windows.h>
#include <webview2.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

typedef struct {
    ICoreWebView2* webView; // Direct WebView2 reference for execution
    const wchar_t* url;
    const wchar_t* method;
    const wchar_t* postData;
    const wchar_t* contentType; // Added for curl -H compatibility
    int isVerbose;
} CommandLineArgs;

// Master Export Signature
__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config);

// Runtime Parsing and Validation Utilities
void PrintUsageInstructions(void);
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_H