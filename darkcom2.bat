@echo off
setlocal DisableDelayedExpansion

title DARK-COM-2 :: STATE BROKER CONTROL PANEL

cd /d "%~dp0"

set "ENDPOINT=https://mercwar01.byethost3.com/darkcom2/index.php"
set "SELECTION=1"
set "ESC="

for /F %%A in ('echo prompt $E ^| cmd') do set "ESC=%%A"

:: 1. Force the terminal application to resize using native ANSI escape codes
:: This sequence works in modern Windows 11 terminals where 'mode con:' fails.
<nul set /p="%ESC%[8;45;86t"

:: Enable ANSI colors on supported Windows terminals.
reg query HKCU\Console /v VirtualTerminalLevel >nul 2>&1
if errorlevel 1 reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul 2>&1

:: Initial full screen clear on startup only
cls

:MENU
:: Snap cursor to top-left (0,0) and wipe all trailing characters to prevent ghosting/overlay
<nul set /p "=%ESC%[H%ESC%[J"

echo %ESC%[96m  ================================================================================%ESC%[0m
echo %ESC%[96m   ____   _    ____  _  __  ____ ___  __  __ ____                                %ESC%[0m
echo %ESC%[96m  ^|  _ \ / \  ^|  _ \^| ^|/ / / ___/ _ \^|  \/  ^|___ \                               %ESC%[0m
echo %ESC%[96m  ^| ^| ^| / _ \ ^| ^|_) ^| ' / ^| ^|  ^| ^| ^| ^| ^|^|/ ^| __) ^|                              %ESC%[0m
echo %ESC%[96m  ^| ^|_^| / ___ \^|  _ ^<^| . \ ^| ^|__^| ^|_^| ^|  ^|  ^|/ __/                            %ESC%[0m
echo %ESC%[96m  ^|____/_/   \_\_^| \_\_^|\_\ \____\___/^|^|  ^|_^|_____^|                             %ESC%[0m
echo %ESC%[96m  ================================================================================%ESC%[0m
echo.
echo   %ESC%[93m              D A R K - C O M - 2%ESC%[0m
echo   %ESC%[97m             STATE BROKER CONTROL PANEL%ESC%[0m
echo   %ESC%[90m             SECURE DATA  ::  SESSION ROUTING%ESC%[0m
echo.
echo   %ESC%[96m  [ SYSTEM ENDPOINT ]%ESC%[0m
echo   %ESC%[90m  %%ENDPOINT%%%ESC%[0m
echo.
echo   %ESC%[96m  ------------------------------------------------------------------------------%ESC%[0m
echo   %ESC%[93m  NAVIGATION%ESC%[0m  %ESC%[97mUP / DOWN%ESC%[0m  %ESC%[90mor%ESC%[0m  %ESC%[97mW / S%ESC%[0m  %ESC%[90m^|%ESC%[0m  %ESC%[97m1-9%ESC%[0m  %ESC%[90mJUMP%ESC%[0m  %ESC%[90m^|%ESC%[0m  %ESC%[97mENTER%ESC%[0m  %ESC%[90mSELECT%ESC%[0m
echo   %ESC%[96m  ==============================================================================%ESC%[0m
echo.


echo.

call :DRAW_OPTION 1 "READ " "APPLICATION STATE" "PUBLIC"
call :DRAW_OPTION 2 "WRITE" "APPLICATION STATE" "PUBLIC"
call :DRAW_OPTION 3 "READ " "APPLICATION STATE" "PRIVATE"
call :DRAW_OPTION 4 "WRITE" "APPLICATION STATE" "PRIVATE"

echo.
echo   %ESC%[90mSESSION OPERATIONS%ESC%[0m
call :DRAW_OPTION 5 "READ " "SESSION STATE    " "PUBLIC"
call :DRAW_OPTION 6 "WRITE" "SESSION STATE    " "PUBLIC"
call :DRAW_OPTION 7 "READ " "SESSION STATE    " "PRIVATE"
call :DRAW_OPTION 8 "WRITE" "SESSION STATE    " "PRIVATE"

echo.
if "%SELECTION%"=="9" (
    echo   %ESC%[41;97m  ^>  EXIT CONTROL PANEL                                                %ESC%[0m
) else (
    echo   %ESC%[97m      EXIT CONTROL PANEL                                                %ESC%[0m
)

echo.
echo   %ESC%[96m------------------------------------------------------------------------------%ESC%[0m
echo   %ESC%[90mHTTP ENGINE:%ESC%[0m darkcom2.exe   %ESC%[90mPROTOCOL:%ESC%[0m curl-compatible options
echo   %ESC%[90mSTATUS:%ESC%[0m Ready
echo.

:: Capture keyboard input including UP/DOWN arrows, W/S, Enter, and Numbers 1-9 via PowerShell
for /f "delims=" %%k in ('powershell -NoProfile -Command "$k = [Console]::ReadKey($true); if ($k.Key -eq 'UpArrow') { 'UP' } elseif ($k.Key -eq 'DownArrow') { 'DOWN' } elseif ($k.Key -eq 'Enter') { 'ENTER' } else { $k.KeyChar.ToString() }"') do set "KEY=%%k"

if /i "%KEY%"=="UP" goto KEY_UP
if /i "%KEY%"=="W" goto KEY_UP
if /i "%KEY%"=="DOWN" goto KEY_DOWN
if /i "%KEY%"=="S" goto KEY_DOWN
if /i "%KEY%"=="ENTER" goto EXECUTE_CHOICE

if "%KEY%"=="1" set "SELECTION=1" & goto EXECUTE_CHOICE
if "%KEY%"=="2" set "SELECTION=2" & goto EXECUTE_CHOICE
if "%KEY%"=="3" set "SELECTION=3" & goto EXECUTE_CHOICE
if "%KEY%"=="4" set "SELECTION=4" & goto EXECUTE_CHOICE
if "%KEY%"=="5" set "SELECTION=5" & goto EXECUTE_CHOICE
if "%KEY%"=="6" set "SELECTION=6" & goto EXECUTE_CHOICE
if "%KEY%"=="7" set "SELECTION=7" & goto EXECUTE_CHOICE
if "%KEY%"=="8" set "SELECTION=8" & goto EXECUTE_CHOICE
if "%KEY%"=="9" set "SELECTION=9" & goto EXECUTE_CHOICE

goto MENU

:KEY_UP
if "%SELECTION%"=="1" (set "SELECTION=9") else (set /a "SELECTION-=1")
goto MENU

:KEY_DOWN
if "%SELECTION%"=="9" (set "SELECTION=1") else (set /a "SELECTION+=1")
goto MENU

:EXECUTE_CHOICE
if "%SELECTION%"=="1" goto READ_APP_PUB
if "%SELECTION%"=="2" goto WRITE_APP_PUB
if "%SELECTION%"=="3" goto READ_APP_PRIV
if "%SELECTION%"=="4" goto WRITE_APP_PRIV
if "%SELECTION%"=="5" goto READ_SES_PUB
if "%SELECTION%"=="6" goto WRITE_SES_PUB
if "%SELECTION%"=="7" goto READ_SES_PRIV
if "%SELECTION%"=="8" goto WRITE_SES_PRIV
if "%SELECTION%"=="9" exit /b 0
goto MENU


:DRAW_OPTION
if "%SELECTION%"=="%~1" (
    echo   %ESC%[30;103m  ^>  [%~1]  %~2  %~3  [%~4]                                     %ESC%[0m
) else (
    echo   %ESC%[97m      [%~1]  %~2  %~3  [%~4]                                     %ESC%[0m
)
exit /b


:: Inline function helper for pretty printing JSON without breaking batch context scope
:DO_PRINT
echo.
echo %ESC%[92m[PARSED RESPONSE]:%ESC%[0m
powershell.exe -NoProfile -Command "$input | ConvertFrom-Json | ConvertTo-Json" 2>nul
if errorlevel 1 echo [Raw Output or Empty Response]
goto :EOF


:READ_APP_PUB
cls
echo.
echo [*] Fetching Application Public State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=true&application=true&write=false" -X GET
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:WRITE_APP_PUB
cls
echo.
set /p "json_data=Enter JSON payload (e.g. {"status":"online"}): "
echo [*] Writing to Application Public State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=true&application=true&write=true" -X POST -d "%json_data%"
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:READ_APP_PRIV
cls
echo.
echo [*] Fetching Application Private State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=false&application=true&write=false" -X GET
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:WRITE_APP_PRIV
cls
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Application Private State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=false&application=true&write=true" -X POST -d "%json_data%"
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:READ_SES_PUB
cls
echo.
echo [*] Fetching Session Public State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=true&application=false&write=false" -X GET
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:WRITE_SES_PUB
cls
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Session Public State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=true&application=false&write=true" -X POST -d "%json_data%"
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:READ_SES_PRIV
cls
echo.
echo [*] Fetching Session Private State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=false&application=false&write=false" -X GET
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU

:WRITE_SES_PRIV
cls
echo.
set /p "json_data=Enter JSON payload: "
echo [*] Writing to Session Private State...
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
darkcom2.exe "%ENDPOINT%?public=false&application=false&write=true" -X POST -d "%json_data%"
echo %ESC%[96m------------------------------------------------------------%ESC%[0m
pause
goto MENU
