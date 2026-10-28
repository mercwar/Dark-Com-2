@echo off
echo ============================================
echo  AIFVS-ARTIFACT: Dark-Com-2.bat [HEADLESS DLL EDITION]
echo  Auto MSVC env + Auto Dependency + Headless DLL Engine
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects
if exist DarkComHeadless.dll del /f /q DarkComHeadless.dll
if exist headless.obj del /f /q headless.obj
if exist resource.res del /f /q resource.res

:: Initialize local compiler dependencies if environment context paths are blank
if not defined VCINSTALLDIR (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
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
        echo [WARN] nuget.exe failed or was absent. Switching to direct package extraction API...
        echo [INFO] Fetching raw Microsoft.Web.WebView2 payload archive stream...
        
        if exist "webview2_pkg.zip" del /f /q webview2_pkg.zip
        
        :: FIXED: Corrected target route pointing straight to the official stable NuGet download stream endpoint API
        curl -s -L -o webview2_pkg.zip "https://nuget.org"
        
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

:: Verify presence of the headless target file
if not exist headless.c (
    echo ERROR: Headless compilation target file 'headless.c' is missing!
    goto end
)

echo Compiling embedded binary resource layout assets...
set "RES_FILE="
if exist favi-con.ico (
    if exist resource.rc (
        rc.exe resource.rc
        if exist resource.res set "RES_FILE=resource.res"
    ) else (
        echo WARNING: resource.rc missing. Skipping resource compilation step...
    )
) else (
    echo WARNING: favi-con.ico was missing! Skipping embedding step...
)

echo Building headless runtime layer: DarkComHeadless.dll

:: Dynamically handle fallback path options if structure unpacks variations
set "LIB_DIR=packages\Microsoft.Web.WebView2\build\native\x64"
if not exist "%LIB_DIR%" set "LIB_DIR=packages\Microsoft.Web.WebView2\build\native\win-x64"

cl ^
    headless.c ^
    /LD ^
    /O2 ^
    /DUNICODE /D_UNICODE /DCINTERFACE ^
    /I"packages\Microsoft.Web.WebView2\build\native\include" ^
    /I"include" ^
    /TC ^
    /link ^
    /DLL ^
    /MACHINE:X64 ^
    /SUBSYSTEM:WINDOWS ^
    /DEF:include/darkcom.def ^
    /LIBPATH:"packages\Microsoft.Web.WebView2\build\native\x64" ^
    WebView2Loader.dll.lib user32.lib ole32.lib gdi32.lib shlwapi.lib shell32.lib advapi32.lib crypt32.lib ^
    /OUT:"DarkComHeadless.dll"

if exist DarkComHeadless.dll (
    echo.
    echo Build Complete: DarkComHeadless.dll
    echo --------------------------------------------
    
    :: Safe extraction transfer routing of the required user engine system loader artifact
    if exist "%LIB_DIR%\WebView2Loader.dll" (
        copy /Y "%LIB_DIR%\WebView2Loader.dll" "WebView2Loader.dll" >nul
    )
    
    echo [SUCCESS] Headless dynamic library compiled cleanly for FireGem allocation loop.
) else (
    echo.
    echo --------------------------------------------
    echo ERROR: Build failed. Verify your compiler flags or asset paths.
    echo --------------------------------------------
)

:end
pause
