@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ============================================
echo   AIFVS-ARTIFACT: Dark-Com-2-dc2ws2.bat [dc2ws2 DLL EDITION]
echo   Auto MSVC env + Custom Winsock DLL Build Engine
echo ============================================

:: Force execution context to lock directly onto the script's home folder location
cd /d "%~dp0"

:: Flush previous operational application states and runtime cached objects for dc2ws2
if exist dc2ws2.dll del /f /q dc2ws2.dll
if exist dc2ws2.lib del /f /q dc2ws2.lib
if exist dc2ws2.exp del /f /q dc2ws2.exp
if exist dc2ws2.obj del /f /q dc2ws2.obj

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

echo ============================================
echo Building Custom Winsock Layer: dc2ws2.dll
echo ============================================

:: Compile dc2ws2 as a shared library DLL including explicit export definitions from include/dc2ws2.def
cl.exe ^
    dc2ws2.c ^
    /LD ^
    /O2 ^
    /D_CRT_SECURE_NO_WARNINGS ^
    /DBUILDING_DC2WS2_DLL ^
    /TC ^
    /I"include" ^
    /link ^
    /DLL ^
    /MACHINE:X64 ^
	/DEF:"include\dc2ws2.def" ^
    ws2_32.lib ^
    /OUT:"dc2ws2.dll"

if exist dc2ws2.dll (
    echo.
    echo --------------------------------------------
    echo Build Complete: dc2ws2.dll
    echo --------------------------------------------
    echo [SUCCESS] dc2ws2.dll compiled cleanly with explicit def export markers.
) else (
    echo.
    echo --------------------------------------------
    echo ERROR: Build failed. Verify your compiler flags or source paths.
    echo --------------------------------------------
)

:end
pause