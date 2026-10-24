@echo off
echo ============================================
echo  AIFVS-ARTIFACT: Dark-Com-2.bat
echo  Auto MSVC env + Compile WebView2 View Engine
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects
if exist Dark-Com-2.exe del /f /q Dark-Com-2.exe
if exist main.obj del /f /q main.obj
if exist resource.res del /f /q resource.res

:: Initialize local compiler dependencies if environment context paths are blank
if not defined VCINSTALLDIR (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
)

echo Compiling embedded binary resource layout assets...
if exist favi-con.ico (
    rc.exe resource.rc
) else (
    echo WARNING: favi-con.ico was missing! Skipping embedding step...
)

echo Building application window layer: Dark-Com-2.exe

:: Compile application code with explicit Unicode definitions to fix string truncation
cl ^
  main.cpp ^
  resource.res ^
  /DUNICODE /D_UNICODE ^
  /I "packages\Microsoft.Web.WebView2\build\native\include" ^
  /Fe:Dark-Com-2.exe ^
  /EHsc ^
  /link ^
  /LIBPATH:"packages\Microsoft.Web.WebView2\build\native\x64" ^
  WebView2Loader.dll.lib user32.lib ole32.lib gdi32.lib

if exist Dark-Com-2.exe (
    echo.
    echo Build Complete: Dark-Com-2.exe
    echo --------------------------------------------
    
    :: Safe extraction transfer routing of the required user engine system loader artifact
    if exist "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" (
        copy /Y "packages\Microsoft.Web.WebView2\build\native\x64\WebView2Loader.dll" "WebView2Loader.dll" >nul
    )
    
    echo Launching Embedded Form Window Instance...
    Dark-Com-2.exe
) else (
    echo.
    echo --------------------------------------------
    echo ERROR: Build failed. Verify your compiler or asset paths.
    echo --------------------------------------------
)

pause
