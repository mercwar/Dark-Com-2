#ifndef HEADLESS_PS_H
#define HEADLESS_PS_H

#ifdef __cplusplus
extern "C" {
#endif

// Data structure to store mapped command line flags parsed from PowerShell
typedef struct {
    const wchar_t* url;
    const wchar_t* method;
    const wchar_t* postData;
    int isVerbose;
} CommandLineArgs;

// Runtime Parsing and Validation Utilities
void PrintUsageInstructions(void);
BOOL ParsePowerShellArguments(int argc, wchar_t* argv[], CommandLineArgs* outArgs);

#ifdef __cplusplus
}
#endif

#endif // HEADLESS_PS_H
