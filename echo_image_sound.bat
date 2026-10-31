@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul
cd /d "%~dp0"

:: Initialize ANSI ESC character for Windows batch colors
for /f %%a in ('echo prompt $e ^| cmd') do set "ESC=%%a"

:: CONFIGURATION LAYER - Stable Live Endpoints (.wav for audio playback)
set "TEXT_URL=http://jsonplaceholder.typicode.com/todos/1"
set "LOCAL_MODE=1"

if "%LOCAL_MODE%"=="0" (
    set "IMAGE_URL=http://localhost:8080/dc2.png"
    set "AUDIO_URL=http://localhost:8080/transformers.mid"
) else (
    set "IMAGE_URL=https://raw.githubusercontent.com/mercwar/dc2url/refs/heads/main/images/dc2.png"
    set "AUDIO_URL=https://raw.githubusercontent.com/mercwar/dc2url/refs/heads/main/sound/transformers.mid"
)

:: Terminal Window Initialization
color 07
mode con: cols=90 lines=36

:: ANSI Color Escape Sequences for Fire and Ice Theme
set "RESET=%ESC%[0m"
set "BOLD=%ESC%[1m"
set "RED=%ESC%[31m"
set "ORANGE=%ESC%[38;5;208m"
set "BLUE=%ESC%[34m"
set "ICE=%ESC%[96m"

cls
echo %BLUE%========================================================================================%RESET%
echo   %BOLD%%ORANGE%■ IGNITION INTEGRATION%RESET%                     %BOLD%%ICE%[FIRE ^& ICE NETWORK PIPELINE]%RESET%
echo %BLUE%========================================================================================%RESET%
echo   %BOLD%DATE:%RESET% %DATE%  ^|  %BOLD%TIME:%RESET% %TIME%  ^|  %BOLD%CORE:%RESET% ASYNCHRONOUS RUNTIME ENVIRONMENT
echo.
echo   %ORANGE%[SYSTEM]%RESET% Deploying localized sockets and evaluating remote streams...
echo.

:: ------------------------------------------------------------------------
:: STAGE 1: JSON TEXT STREAM
:: ------------------------------------------------------------------------
echo   %BOLD%%RED%[STAGE 01 / 03]%RESET% %BOLD%%ORANGE%FIRE VECTOR — TEXT STREAM%RESET%
echo   Connection Target : %BLUE%%TEXT_URL%%RESET%
if exist test_text.json del /f /q test_text.json
echo   Network Status    : Polling target host stream allocation loops...
dc2url.exe -l "%TEXT_URL%" -nojs -o "%~dp0test_text.json" >nul 2>&1

if not exist test_text.json goto STAGE1_FAIL
for %%A in (test_text.json) do set "tsize=%%~zA"
if "%tsize%"=="" set "tsize=0"
if %tsize% LEQ 0 goto STAGE1_FAIL

echo   Validation Result : %ORANGE%SUCCESS%RESET% ^| Committed: %BOLD%%tsize% Bytes%RESET%
echo   Stream Output     :
for /f "usebackq delims=" %%i in ("test_text.json") do echo      %ICE%^>%RESET% %%i
goto STAGE2_START

:STAGE1_FAIL
echo   Validation Result : %RED%FAILED%RESET% ^| Reason: Pipeline terminated or null payload received.

:: ------------------------------------------------------------------------
:: STAGE 2: BINARY IMAGE ARRAY
:: ------------------------------------------------------------------------
:STAGE2_START
echo.
echo   %BOLD%%ICE%[STAGE 02 / 03]%RESET% %BOLD%%ICE%ICE VECTOR — BINARY IMAGE MATRIX%RESET%
echo   Connection Target : %BLUE%%IMAGE_URL%%RESET%
if exist dc2.png del /f /q dc2.png
echo   Network Status    : Freezing connection frames and unmapping headers...
dc2url.exe -l "%IMAGE_URL%" -nojs -o "%~dp0dc2.png" >nul 2>&1

ping 127.0.0.1 -n 2 >nul

if not exist dc2.png goto STAGE2_FAIL
for %%B in (dc2.png) do set "isize=%%~zB"
if "%isize%"=="" set "isize=0"
if %isize% LEQ 100 goto STAGE2_FAIL

echo   Validation Result : %ICE%SUCCESS%RESET% ^| Stored: [dc2.png] (%isize% Bytes)

:: Automatically launch and display the downloaded image
start "" "%~dp0dc2.png"

goto STAGE3_START

:STAGE2_FAIL
echo   Validation Result : %RED%FAILED%RESET% ^| Reason: Transmission sockets unmapped inside structural loop.

:: ------------------------------------------------------------------------
:: STAGE 3: AUDIO DATA STREAM (Fast-path HTTP Vector)
:: ------------------------------------------------------------------------
:STAGE3_START
echo.
echo   %BOLD%%RED%[STAGE 03 / 03]%RESET% %BOLD%%ORANGE%FIRE VECTOR — AUDIO METADATA%RESET%
:: Use a verified live HTTP audio endpoint

echo   Connection Target : %BLUE%%AUDIO_URL%%RESET%
if exist transformers.mid del /f /q transformers.mid
echo   Network Status    : Burning media segments into active temporary storage...

:: Fixed argument quotes and matching .wav destination
dc2url.exe -l "%AUDIO_URL%" -nojs -o "%~dp0transformers.mid" >nul 2>&1

ping 127.0.0.1 -n 2 >nul

if not exist transformers.mid goto STAGE3_FAIL
for %%C in (transformers.mid) do set "asize=%%~zC"
if "%asize%"=="" set "asize=0"
if %asize% LEQ 1000 goto STAGE3_FAIL

echo   Validation Result : %ORANGE%SUCCESS%RESET% ^| Cache: [transformers.mid] (%asize% Bytes)

:: Automatically play the downloaded audio asset natively
start "" "%~dp0transformers.mid"
goto COMPLETE

:STAGE3_FAIL
echo   Validation Result : %RED%FAILED%RESET% ^| Reason: Handshake negotiation failed or domain resolution dropped.
:: ------------------------------------------------------------------------
:: FINALIZE
:: ------------------------------------------------------------------------
:COMPLETE
echo.
echo %BLUE%========================================================================================%RESET%
echo   %ICE%[COMPLETE]%RESET% Engine cycle finalized. Active tracking sequences closed down cleanly.
echo %BLUE%========================================================================================%RESET%
echo.
pause
