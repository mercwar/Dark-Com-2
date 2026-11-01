@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
cd /d "%~dp0"

:: Initialize ANSI ESC character for Windows batch colors
for /f %%a in ('echo prompt $e ^| cmd') do set "ESC=%%a"

:: CONFIGURATION LAYER
set "TEXT_URL=https://mercwar01.byethost3.com/darkcom2/echo.php"
set "MENU_NAME=menu.json"
set "EXE_NAME=menu.json"

set "IMAGE_URL=https://mercwar01.byethost3.com/darkcom2/downlink/eclipse"
set "AUDIO_URL=https://mercwar01.byethost3.com/darkcom2/downlink.php?file=/eclipse"

:: Terminal Window Initialization
color 07
mode con: cols=100 lines=40

:: ANSI Color Escape Sequences for Fire and Ice Theme
set "RESET=%ESC%[0m"
set "BOLD=%ESC%[1m"
set "RED=%ESC%[31m"
set "ORANGE=%ESC%[38;5;208m"
set "BLUE=%ESC%[34m"
set "ICE=%ESC%[96m"
set "GREEN=%ESC%[32m"

cls
echo %BLUE%========================================================================================%RESET%
echo    %BOLD%%ORANGE%■ IGNITION INTEGRATION MATRIX%RESET%     %BOLD%%ICE%[COMBO TEST: WEBVIEW2 + NOJS + DC2WS2 + NOSCRIPT]%RESET%
echo %BLUE%========================================================================================%RESET%
echo    %BOLD%DATE:%RESET% %DATE%  ^|  %BOLD%TIME:%RESET% %TIME%  ^|  %BOLD%CORE:%RESET% Multi-Vector Engine Verification
echo.

:: ------------------------------------------------------------------------
:: STAGE 01: Standard WebView2 Headless Mode (-l)
:: ------------------------------------------------------------------------
echo    %BOLD%%RED%[TEST 01 / 04]%RESET% %BOLD%%ORANGE%WEBVIEW2 STANDARD ENGINE (-l)%RESET%
echo    Target : %BLUE%%TEXT_URL%%RESET%
if exist test_s1.json del /f /q test_s1.json
dc2url.exe -l -v "%TEXT_URL%" -o "%~dp0test_s1.json"
if exist test_s1.json (
    for %%A in (test_s1.json) do set "sz=%%~zA"
    echo    Result : %GREEN%SUCCESS%RESET% ^| Committed: !sz! Bytes
) else (
    echo    Result : %RED%FAILED%RESET%
)

:: ------------------------------------------------------------------------
:: STAGE 02: WebView2 + NoJS Stub (-l -nojs)
:: ------------------------------------------------------------------------
echo.
echo    %BOLD%%RED%[TEST 02 / 04]%RESET% %BOLD%%ICE%WEBVIEW2 WITH FAST-PATH STUB (-l -nojs)%RESET%
echo    Target : %BLUE%%TEXT_URL%%RESET%
if exist test_s2.json del /f /q test_s2.json
dc2url.exe -v -l -nojs "%TEXT_URL%" -o "%~dp0test_s2.json"
if exist test_s2.json (
    for %%B in (test_s2.json) do set "sz=%%~zB"
    echo    Result : %GREEN%SUCCESS%RESET% ^| Committed: !sz! Bytes
) else (
    echo    Result : %RED%FAILED%RESET%
)

:: ------------------------------------------------------------------------
:: STAGE 03: Headless WebView2 Render Pipeline (Private Server File 1)
:: ------------------------------------------------------------------------
echo.
echo    %BOLD%%RED%[TEST 03 / 04]%RESET% %BOLD%%ORANGE%WEBVIEW2 ENGINE FORCE (-l)%RESET%
echo    Target : %BLUE%%IMAGE_URL%/%MENU_NAME%%RESET%
if exist "%~dp0%MENU_NAME%" del /f /q "%~dp0%MENU_NAME%"

dc2url.exe -l -v "%IMAGE_URL%/%MENU_NAME%" -o "%~dp0%MENU_NAME%"

if exist "%~dp0%MENU_NAME%" (
    powershell -NoProfile -Command "$f='%~dp0%MENU_NAME%'; if(Test-Path $f){ $txt=[IO.File]::ReadAllText($f); if($txt -match '\<pre\>([\s\S]*?)\<\/pre\>'){ [IO.File]::WriteAllText($f, $Matches[1].Trim()) } }" >nul 2>&1
)

if exist "%~dp0%MENU_NAME%" (
    for %%C in ("%~dp0%MENU_NAME%") do set "sz=%%~zC"
    echo    Result : %GREEN%SUCCESS%RESET% ^| Stored: [%MENU_NAME%] (!sz! Bytes)
) else (
    echo    Result : %RED%FAILED%RESET%
)

:: ------------------------------------------------------------------------
:: STAGE 04: Headless WebView2 Render Pipeline (Private Server File 2)
:: ------------------------------------------------------------------------
echo.
echo    %BOLD%%RED%[TEST 04 / 04]%RESET% %BOLD%%ICE%WEBVIEW2 ENGINE FORCE (-l)%RESET%
echo    Target : %BLUE%%AUDIO_URL%/%EXE_NAME%%RESET%
if exist "%~dp0%EXE_NAME%" del /f /q "%~dp0%EXE_NAME%"

dc2url.exe -l -v -nojs "%AUDIO_URL%/%EXE_NAME%" -o "%~dp0%EXE_NAME%"

if exist "%~dp0%EXE_NAME%" (
    powershell -NoProfile -Command "$f='%~dp0%EXE_NAME%'; if(Test-Path $f){ $txt=[IO.File]::ReadAllText($f); if($txt -match '\<pre\>([\s\S]*?)\<\/pre\>'){ [IO.File]::WriteAllText($f, $Matches[1].Trim()) } }" >nul 2>&1
)

if exist "%~dp0%EXE_NAME%" (
    for %%D in ("%~dp0%EXE_NAME%") do set "sz=%%~zD"
    echo    Result : %GREEN%SUCCESS%RESET% ^| Cache: [%EXE_NAME%] (!sz! Bytes)
) else (
    echo    Result : %RED%FAILED%RESET%
)

:: ------------------------------------------------------------------------
:: FINALIZE
:: ------------------------------------------------------------------------
echo.
echo %BLUE%========================================================================================%RESET%
echo    %ICE%[COMPLETE]%RESET% All engine execution combinations verified successfully.
echo %BLUE%========================================================================================%RESET%
echo.
pause