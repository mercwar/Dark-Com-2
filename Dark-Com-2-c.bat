@echo off
echo ============================================
echo  AIFVS-ARTIFACT: Dark-Com-2.bat [C EDITION]
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
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Visual Studio vcvarsall.bat environment initializer script not found.
        goto end
    )
)

:: Validate package folder structure before trying to target compilation paths
if not exist "packages\Microsoft.Web.WebView2" (
    echo ERROR: WebView2 NuGet dependency package is missing in 'packages\' directory.
    goto end
)

:: Verify presence of the base input main.c source architecture
if not exist main.c (
    echo ERROR: Base compilation target file 'main.c' is missing!
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

echo Building application window layer: Dark-Com-2.exe

:: Compile pure C code, moving the resource file down to the link stage parameters
cl ^
  main.c ^
  /DUNICODE /D_UNICODE /DCINTERFACE ^
  /I"packages\Microsoft.Web.WebView2\build\native\include" ^
  /Fe"Dark-Com-2.exe" ^
  /TC ^
  /link ^
  %RES_FILE% ^
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

:end
pause
