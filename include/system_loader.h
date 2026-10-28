#ifndef SYSTEM_LOADER_H
#define SYSTEM_LOADER_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Function pointer signature matching DarkComHeadless exports
typedef int (*pfnRunHeadlessInit)(void);
typedef int (*pfnRunHeadlessRequest)(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize);

// Resolves and loads DarkComHeadless.dll using registry mapping or local fallback
HMODULE LoadSystemDarkComHeadless(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_LOADER_H