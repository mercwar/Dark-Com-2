@echo off
setlocal DisableDelayedExpansion

:: ============================================================================
:: Dark-Com-2 Interactive State Broker Menu (menu.bat)
:: ============================================================================

set "ENDPOINT=https://iblogger.org"
set "APP_EXE=%~dp0darkcom2.exe"

:MENU
cls
echo ============================================================
echo         DARK-COM-2 STATE BROKER CONTROL PANEL
echo ============================================================
echo  [1] Read Application (Public)
echo  [2] Write Application (Public)
echo  [3] Read Application (Private)
echo  [4] Write Application (Private)
echo  [5] Read Session (Public)
echo  [6] Write Session (Public)
echo  [7] Read Session (Private)
echo  [8] Write Session (Private)
echo  [9] Exit
echo ============================================================
set /p "choice=Select option [1-9]: "

if "%choice%"=="1" goto READ_APP_PUB
if "%choice%"=="2" goto WRITE_APP_PUB
if "%choice%"=="3" goto READ_APP_PRIV
if "%choice%"=="4" goto WRITE_APP_PRIV
if "%choice%"=="5" goto READ_SES_PUB
if "%choice%"=="6" goto WRITE_SES_PUB
if "%choice%"=="7" goto READ_SES_PRIV
if "%choice%"=="8" goto WRITE_SES_PRIV
if "%choice%"=="9" exit /b 0

goto MENU

:CHECK_EXE
if not exist "%APP_EXE%" (
    echo.
    echo [!] ERROR: Executable missing.
    echo Expected file at: "%APP_EXE%"
    echo Please make sure darkcom2.exe is copied into that folder.
    echo ------------------------------------------------------------
    pause
    goto MENU
)
goto :EOF

:READ_APP_PUB
call :CHECK_EXE
echo.
echo [*] Fetching Application Public State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=true&application=true&write=false" -X GET
echo ------------------------------------------------------------
pause
goto MENU

:WRITE_APP_PUB
call :CHECK_EXE
echo.
set /p "json_data=Enter JSON payload (e.g. {"status":"online"}): "
echo [*] Writing to Application Public State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=true&application=true&write=true" -X POST -d "%json_data%"
echo ------------------------------------------------------------
pause
goto MENU

:READ_APP_PRIV
call :CHECK_EXE
echo.
echo [*] Fetching Application Private State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=false&application=true&write=false" -X GET
echo ------------------------------------------------------------
pause
goto MENU

:WRITE_APP_PRIV
call :CHECK_EXE
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Application Private State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=false&application=true&write=true" -X POST -d "%json_data%"
echo ------------------------------------------------------------
pause
goto MENU

:READ_SES_PUB
call :CHECK_EXE
echo.
echo [*] Fetching Session Public State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=true&application=false&write=false" -X GET
echo ------------------------------------------------------------
pause
goto MENU

:WRITE_SES_PUB
call :CHECK_EXE
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Session Public State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=true&application=false&write=true" -X POST -d "%json_data%"
echo ------------------------------------------------------------
pause
goto MENU

:READ_SES_PRIV
call :CHECK_EXE
echo.
echo [*] Fetching Session Private State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=false&application=false&write=false" -X GET
echo ------------------------------------------------------------
pause
goto MENU

:WRITE_SES_PRIV
call :CHECK_EXE
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Session Private State...
echo ------------------------------------------------------------
"%APP_EXE%" "%ENDPOINT%?public=false&application=false&write=true" -X POST -d "%json_data%"
echo ------------------------------------------------------------
pause
goto MENU
