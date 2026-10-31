#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "system_loader.h"

HMODULE LoadSystemDarkComHeadless(int forceLocalDir) {
    wchar_t targetPath[MAX_PATH] = {0};
    HMODULE hDll = NULL;

    // 1. If the local flag is set, load directly from dc2url.exe's containing folder first
    if (forceLocalDir) {
        wchar_t currentExe[MAX_PATH] = {0};
        if (GetModuleFileNameW(NULL, currentExe, MAX_PATH) > 0) {
            wchar_t* lastSlash = wcsrchr(currentExe, L'\\');
            if (lastSlash) {
                *lastSlash = L'\0';
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", currentExe);
                
                SetDllDirectoryW(currentExe);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                SetDllDirectoryW(NULL);
            }
        }
    }

    // 2. Fallback / Standard: Query registry path centralized at HKCU\Software\Eclipse\Launcher\InstallPath
    if (!hDll) {
        wchar_t installPath[MAX_PATH] = {0};
        DWORD bufferSize = sizeof(installPath);
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Eclipse\\Launcher", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, L"InstallPath", NULL, NULL, (LPBYTE)installPath, &bufferSize) == ERROR_SUCCESS) {
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", installPath);
                
                SetDllDirectoryW(installPath);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                SetDllDirectoryW(NULL);
            }
            RegCloseKey(hKey);
        }
    }

    // 3. Fallback: Check local execution path automatically if not already tried
    if (!hDll && !forceLocalDir) {
        wchar_t currentExe[MAX_PATH] = {0};
        if (GetModuleFileNameW(NULL, currentExe, MAX_PATH) > 0) {
            wchar_t* lastSlash = wcsrchr(currentExe, L'\\');
            if (lastSlash) {
                *lastSlash = L'\0';
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", currentExe);
                
                SetDllDirectoryW(currentExe);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                SetDllDirectoryW(NULL);
            }
        }
    }

    // 4. Last resort fallback to current working directory
    if (!hDll) {
        hDll = LoadLibraryExW(L"DarkComHeadless.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    return hDll;
}