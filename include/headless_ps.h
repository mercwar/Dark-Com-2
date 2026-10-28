#ifndef DC2URL_H
#define DC2URL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prototype for registry-backed system loader (defined in system_loader.c)
HMODULE LoadSystemDarkComHeadless(void);

#ifdef __cplusplus
}
#endif

#endif // DC2URL_H