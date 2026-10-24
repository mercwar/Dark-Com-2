@echo off
echo ============================================
echo  AIFVS-ARTIFACT: Dark-Com-2.bat [POWERSHELL CONSOLE EDITION]
echo  Auto MSVC env + Auto Dependency + headless_ps Command App
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects
if exist darkcom2.exe del /f /q darkcom2.exe
if exist headless_ps.obj del /f /q headless_ps.obj
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
        echo [WARN] nuget.exe failed or was blocked by the OS. Switching to native web extraction...
        echo [INFO] Fetching Microsoft.Web.WebView2 payload from official upstream repository...
        
        if exist "webview2_pkg.zip" del /f /q webview2_pkg.zip
        
        :: Routing target explicitly to binary package stream API endpoint instead of web landing page
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

:: Verify presence of the console target files
if not exist headless_ps.c (
    echo ERROR: PowerShell console app script file 'headless_ps.c' is missing!
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

echo Building PowerShell command-line wrapper app: darkcom2.exe

:: Completed cl compilation matrix block for building the console binary executable
cl ^
  headless_ps.c ^
  /O2 ^
  /DUNICODE /D_UNICODE ^
  /TC ^
  /I"include" ^
  /link ^
  %RES_FILE% ^
  /SUBSYSTEM:CONSOLE ^
  user32.lib ole32.lib shlwapi.lib ^
  /OUT:"darkcom2.exe"


if exist darkcom2.exe (
    echo.
    echo Build Complete: darkcom2.exe
    echo --------------------------------------------
    
    :: Extraction transfer routing of the required user engine system loader artifact
    if exist "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" (
        copy /Y "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" "WebView2Loader.dll" >nul
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
