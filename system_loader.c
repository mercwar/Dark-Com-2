#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "system_loader.h"

HMODULE LoadSystemDarkComHeadless(int forceLocalDir) {
    wchar_t targetPath[MAX_PATH] = {0};
    wchar_t ws2TargetPath[MAX_PATH] = {0};
    HMODULE hDll = NULL;
    HMODULE hWs2Dll = NULL;

    // 1. Forced folder route: load directly from dc2url.exe's containing directory first
    if (forceLocalDir) {
        wchar_t currentExe[MAX_PATH] = {0};
        if (GetModuleFileNameW(NULL, currentExe, MAX_PATH) > 0) {
            wchar_t* lastSlash = wcsrchr(currentExe, L'\\');
            if (lastSlash) {
                *lastSlash = L'\0';
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", currentExe);
                swprintf_s(ws2TargetPath, MAX_PATH, L"%s\\dc2ws2.dll", currentExe);
                
                // Retain directory context for secondary dependencies
                SetDllDirectoryW(currentExe);
                
                // Pre-load the high-performance network engine layer into the process context
                hWs2Dll = LoadLibraryExW(ws2TargetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                
                if (hDll) {
                    SetDllDirectoryW(NULL);
                    return hDll;
                }
                SetDllDirectoryW(NULL);
            }
        }
    }

    // 2. Fallback: Query centralized registry target path at HKCU\Software\Eclipse\Launcher\InstallPath
    if (!hDll) {
        wchar_t installPath[MAX_PATH] = {0};
        DWORD bufferSize = sizeof(installPath);
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Eclipse\\Launcher", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, L"InstallPath", NULL, NULL, (LPBYTE)installPath, &bufferSize) == ERROR_SUCCESS) {
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", installPath);
                swprintf_s(ws2TargetPath, MAX_PATH, L"%s\\dc2ws2.dll", installPath);
                
                SetDllDirectoryW(installPath);
                hWs2Dll = LoadLibraryExW(ws2TargetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                
                if (hDll) {
                    RegCloseKey(hKey);
                    SetDllDirectoryW(NULL);
                    return hDll;
                }
            }
            RegCloseKey(hKey);
            SetDllDirectoryW(NULL);
        }
    }

    // 3. Fallback: Automatically search execution workspace folder paths
    if (!hDll && !forceLocalDir) {
        wchar_t currentExe[MAX_PATH] = {0};
        if (GetModuleFileNameW(NULL, currentExe, MAX_PATH) > 0) {
            wchar_t* lastSlash = wcsrchr(currentExe, L'\\');
            if (lastSlash) {
                *lastSlash = L'\0';
                swprintf_s(targetPath, MAX_PATH, L"%s\\DarkComHeadless.dll", currentExe);
                swprintf_s(ws2TargetPath, MAX_PATH, L"%s\\dc2ws2.dll", currentExe);
                
                SetDllDirectoryW(currentExe);
                hWs2Dll = LoadLibraryExW(ws2TargetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                hDll = LoadLibraryExW(targetPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                
                if (hDll) {
                    SetDllDirectoryW(NULL);
                    return hDll;
                }
                SetDllDirectoryW(NULL);
            }
        }
    }

    // 4. Last resort: Fallback onto active current working directory parameters
    if (!hDll) {
        hWs2Dll = LoadLibraryExW(L"dc2ws2.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        hDll = LoadLibraryExW(L"DarkComHeadless.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    (void)hWs2Dll; // Prevent compiler optimization pruning warnings
    return hDll;
}
