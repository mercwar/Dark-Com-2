#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "system_loader.h"

HMODULE LoadSystemDarkComHeadless(void) {
    wchar_t installPath[MAX_PATH] = {0};
    DWORD bufferSize = sizeof(installPath);
    HMODULE hDll = NULL;

    // 1. Query registry path centralized at HKCU\Software\Eclipse\Launcher\InstallPath
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Eclipse\\Launcher", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"InstallPath", NULL, NULL, (LPBYTE)installPath, &bufferSize) == ERROR_SUCCESS) {
            wchar_t dllFullPath[MAX_PATH];
            swprintf_s(dllFullPath, MAX_PATH, L"%s\\DarkComHeadless.dll", installPath);
            hDll = LoadLibraryW(dllFullPath);
        }
        RegCloseKey(hKey);
    }

    // 2. Fallback: Check local execution path or standard subfolders
    if (!hDll) {
        wchar_t currentExe[MAX_PATH];
        GetModuleFileNameW(NULL, currentExe, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(currentExe, L'\\');
        if (lastSlash) *lastSlash = L'\0';

        wchar_t localDll[MAX_PATH];
        swprintf_s(localDll, MAX_PATH, L"%s\\DarkComHeadless.dll", currentExe);
        hDll = LoadLibraryW(localDll);
    }

    // 3. Last resort fallback to working directory
    if (!hDll) {
        hDll = LoadLibraryW(L"DarkComHeadless.dll");
    }

    return hDll;
}