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

// Master Export Signature updated to mirror structure reference patterns
__declspec(dllexport) const char* __cdecl RunHeadlessRequest(const CommandLineArgs* config);

// Runtime Parsing and Validation Utilities
void PrintUsageInstructions(void);
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_H
