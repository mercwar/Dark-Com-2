#ifndef DARK_COM_HEADLESS_H
#define DARK_COM_HEADLESS_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_DARKCOM_DLL
#define DARKCOM_API __declspec(dllexport)
#else
#define DARKCOM_API __declspec(dllimport)
#endif

// Core engine initialization
DARKCOM_API int RunHeadlessInit(void);

// Main headless request execution handler
DARKCOM_API int RunHeadlessRequest(const char* url, const char* method, const char* postData, char* outBuffer, DWORD outBufferSize);

// Engine shutdown cleanup
DARKCOM_API void RunHeadlessShutdown(void);

#ifdef __cplusplus
}
#endif

#endif // DARK_COM_HEADLESS_H