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
    const wchar_t* url;
    const wchar_t* method;
    const wchar_t* postData;
    const wchar_t* contentType;
    int isVerbose;
} CommandLineArgs;

__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config);

void PrintUsageInstructions(void);
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_H