#ifndef HEADLESS_H
#define HEADLESS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Command line configuration structure
typedef struct {
    wchar_t* url;
    wchar_t* method;
    wchar_t* postData;
    int isVerbose;
} CommandLineArgs;

// Master Export Signature for DarkComHeadless.dll
__declspec(dllexport) const char* RunHeadlessRequest(const wchar_t* url, const wchar_t* postData);

// Runtime Parsing and Validation Utilities
void PrintUsageInstructions(void);
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_H