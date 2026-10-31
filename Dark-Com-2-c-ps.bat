@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ============================================
echo   AIFVS-ARTIFACT: Dark-Com-2-c-ps.bat [POWERSHELL CONSOLE EDITION]
echo   Auto MSVC env + Auto Dependency + headless_ps Command App
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects
if exist dc2url.exe del /f /q dc2url.exe
if exist headless_ps.obj del /f /q headless_ps.obj
if exist system_loader.obj del /f /q system_loader.obj
if exist resource.res del /f /q resource.res

:: Initialize local compiler dependencies if environment context paths are blank
if not defined VCINSTALLDIR (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Visual Studio vcvarsall.bat environment initializer script not found.
        goto end
    )
)

:: Self-Healing Check: Auto-resolve missing package directories
if not exist "packages\Microsoft.Web.WebView2" (
    echo [INFO] WebView2 NuGet directory missing. Attempting restoration...
    
    :: Attempt to use the local nuget.exe first
    if exist "nuget.exe" (
        echo [INFO] Testing internal nuget.exe restoration layout...
        "nuget.exe" install Microsoft.Web.WebView2 -OutputDirectory packages -ExcludeVersion >nul 2>&1
    )
    
    :: Fallback: Direct API target download if local executable tracking encounters OS blocks
    if not exist "packages\Microsoft.Web.WebView2" (
        echo [WARN] nuget.exe failed or was blocked by the OS. Switching to native web extraction...
        echo [INFO] Fetching Microsoft.Web.WebView2 payload from official upstream repository...
        
        if exist "webview2_pkg.zip" del /f /q webview2_pkg.zip
        
        :: NuGet V2 direct package download endpoint stream URL
        curl -s -L -o webview2_pkg.zip "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2"
        
        if exist "webview2_pkg.zip" (
            echo [INFO] Extracting package layout archives natively...
            powershell -Command "Expand-Archive -Path 'webview2_pkg.zip' -DestinationPath 'packages\Microsoft.Web.WebView2' -Force"
            del /f /q webview2_pkg.zip
        )
    )
)

:: Re-verify package directory presence after recovery options execute
if not exist "packages\Microsoft.Web.WebView2" (
    echo ERROR: Failed to restore or resolve the WebView2 package dependency directory tree.
    goto end
)

:: Verify presence of the console target files
if not exist headless_ps.c (
    echo ERROR: PowerShell console app script file 'headless_ps.c' is missing!
    goto end
)

if not exist system_loader.c (
    echo ERROR: System loader source file 'system_loader.c' is missing!
    goto end
)

echo Compiling embedded binary resource layout assets...
set "RES_FILE="
if exist favi-con.ico (
    if exist resource.rc (
        :: Compile the dedicated executable resource file into resource.res
        rc.exe /fo resource.res resource.rc
        if exist resource.res set "RES_FILE=resource.res"
    ) else (
        echo WARNING: resource.rc missing. Skipping resource compilation step...
    )
) else (
    echo WARNING: favi-con.ico was missing! Skipping embedding step...
)

echo Building PowerShell command-line wrapper app: dc2url.exe

:: Dynamically calculate library path tracking matrices
set "LIB_DIR=packages\Microsoft.Web.WebView2\build\native\x64"
if not exist "%LIB_DIR%\WebView2Loader.dll.lib" (
    if exist "packages\Microsoft.Web.WebView2\build\native\win-x64\WebView2Loader.dll.lib" (
        set "LIB_DIR=packages\Microsoft.Web.WebView2\build\native\win-x64"
    )
)

:: Completed cl compilation matrix block with WebView2 include/lib paths included
cl.exe ^
    headless_ps.c ^
    system_loader.c ^
    /O2 ^
    /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS ^
    /TC ^
    /I"packages\Microsoft.Web.WebView2\build\native\include" ^
    /I"include" ^
    /link ^
    %RES_FILE% ^
    /LIBPATH:"%LIB_DIR%" ^
    WebView2Loader.dll.lib ^
    /SUBSYSTEM:CONSOLE ^
    user32.lib ole32.lib advapi32.lib crypt32.lib shlwapi.lib shell32.lib ^
    /OUT:"dc2url.exe"

if exist dc2url.exe (
    echo.
    echo Build Complete: dc2url.exe
    echo --------------------------------------------
    
    :: Extraction transfer routing of the required user engine system loader artifact
    if exist "%LIB_DIR%\WebView2Loader.dll" (
        copy /Y "%LIB_DIR%\WebView2Loader.dll" "WebView2Loader.dll" >nul
        echo [SUCCESS] WebView2Loader.dll copied to destination workspace.
    ) else (
        echo [WARN] WebView2Loader.dll was missing inside fallback path definitions!
    )
    
    echo [SUCCESS] Standalone PowerShell CLI client compiled cleanly for terminal orchestration loops.
) else (
    echo.
    echo --------------------------------------------
    echo ERROR: Build failed. Verify your compiler flags or asset paths.
    echo --------------------------------------------
)

:end
pause