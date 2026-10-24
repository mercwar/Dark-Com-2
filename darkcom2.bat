@echo off
setlocal DisableDelayedExpansion

:: ============================================================================
:: GATEWAY CONTROL CONSOLE :: STATE BROKER PANEL (AVIS-2026 PROTOCOL)
:: ============================================================================
title GATEWAY [ACTIVE] - ECLIPSE CONTROL PANEL
color 0B
cd /d "%~dp0"

set "ENDPOINT=https://mercwar01.byethost3.com/darkcom2/index.php"
set "SELECTION=1"
set "KEY="
set "JSON_DATA="
set "REQUEST_URL="
set "REQUEST_METHOD="
set "REQUEST_BODY="
set "OUTFILE=%TEMP%\gateway_response_%RANDOM%.txt"

for /F %%A in ('echo prompt $E ^| cmd') do set "ESC=%%A"

:: Request ANSI support for classic Windows Console.
reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul 2>&1

:: Ask supported modern terminals to use the desired dimensions.
<nul set /p "=%ESC%[8;45;86t"

:: ============================================================
:: AUTO-TEST GATEWAY ON LOAD (READ APPLICATION PUBLIC)
:: ============================================================
cls
echo.
echo %ESC%[96m  [~] Starting Eclips version 1.0..%ESC%[0m
echo.
set "REQUEST_LABEL=AUTO-TEST: READ APPLICATION PUBLIC"
set "REQUEST_URL=%ENDPOINT%?public=true&application=true&write=false"
call :RUN_GET_SILENT
echo %ESC%[92m  [+] The Eclipse Gateway is ALIVE, Loading... %ESC%[0m
timeout /t 2 >nul

cls
goto MENU


:: ============================================================
:: BUFFERED MENU RENDERER
:: Build the complete frame first, then write it in one call.
:: ============================================================

:MENU
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$e=[char]27; $s=[int]$env:SELECTION; $c=$e+'[0m'; $f=New-Object 'System.Collections.Generic.List[string]'; $f.Add($e+'[H'+$e+'[2J'); $f.Add($e+'[96m  ============================================================================'+$c); $f.Add($e+'[96m   ____    _    ____  _  __  ____ ___  __  __ ____'+$c); $f.Add($e+'[96m   ^|  _ \\  / \\   ^|  _ \\^| ^|/ / / ___/ _ \\^|  \\/  ^|___ \\'+$c); $f.Add($e+'[96m   ^| ^| ^| / _ \\  ^| ^|_) ^| '' / ^| ^|    ^| ^| ^| ^|\\/^| ^| __) /'+$c); $f.Add($e+'[96m   ^| ^|_^| / ___ \\^|  _ <^| . \\ ^| ^|__^| ^|_^| ^|    ^| ^|/ __/ '+$c); $f.Add($e+'[96m   ^|____/_/   \\_\\_^| ^|\\_\\_\\_\\ \\____\\___/^|_^|  ^|_____^|'+$c); $f.Add($e+'[96m  ============================================================================'+$c); $f.Add(''); $f.Add($e+'[93m             G A T E W A Y   C O N T R O L'+$c); $f.Add($e+'[97m             ECLIPSE version 1.0'+$c); $f.Add($e+'[90m             AVIS PROTOCOL  ::  SESSION ROUTING'+$c); $f.Add(''); $f.Add($e+'[96m  [ GATEWAY ENDPOINT ]'+$c); $f.Add($e+'[90m   '+$env:ENDPOINT+$c); $f.Add(''); $f.Add($e+'[96m  --------------------------------------------------------------------------'+$c); $f.Add($e+'[93m  NAVIGATION'+$c+'  '+$e+'[97mUP / DOWN'+$c+'  '+$e+'[90mor'+$c+'  '+$e+'[97mW / S'+$c+'  '+$e+'[90m|'+$c+'  '+$e+'[97m1-9'+$c+'  '+$e+'[90mJUMP'+$c+'  '+$e+'[90m|'+$c+'  '+$e+'[97mENTER'+$c+'  '+$e+'[90mSELECT'+$c); $f.Add($e+'[96m  ==========================================================================v'+$c); $f.Add(''); $labels=@('READ  APPLICATION STATE  [PUBLIC]','WRITE APPLICATION STATE  [PUBLIC]','READ  APPLICATION STATE  [PRIVATE]','WRITE APPLICATION STATE  [PRIVATE]','READ  SESSION STATE        [PUBLIC]','WRITE SESSION STATE        [PUBLIC]','READ  SESSION STATE        [PRIVATE]','WRITE SESSION STATE        [PRIVATE]','EXIT GATEWAY CONSOLE'); for($i=1;$i -le 9;$i++){if($i -eq 5){$f.Add('');$f.Add($e+'[90m  SESSION OPERATIONS'+$c)}; $p = if($i -eq $s) { '  >    ['+$i+']   ' } else { '     ['+$i+']   ' }; $line = ($p + $labels[$i-1]).PadRight(74); if($i -eq $s){$f.Add($e+'[30;103m'+$line+$c)}else{$f.Add($e+'[97m'+$line+$c)}}; $f.Add(''); $f.Add($e+'[96m  --------------------------------------------------------------------------'+$c); $f.Add($e+'[90m  HTTP ENGINE:'+ $c+' darkcom2.exe   '+$e+'[90mPROTOCOL:'+ $c+' curl-compatible options'); $f.Add($e+'[90m  STATUS:'+ $c+' Gateway Online'); [Console]::Write($f -join [Environment]::NewLine)"



for /f "delims=" %%K in ('powershell.exe -NoProfile -Command "$k=[Console]::ReadKey($true); if($k.Key -eq [ConsoleKey]::UpArrow){[Console]::WriteLine([char[]](85,80))} elseif($k.Key -eq [ConsoleKey]::DownArrow){[Console]::WriteLine([char[]](68,79,87,78))} elseif($k.Key -eq [ConsoleKey]::Enter){[Console]::WriteLine([char[]](69,78,84,69,82))} else {$n=[int]$k.KeyChar; if($n -ge 97 -and $n -le 122){$n-=32}; [Console]::WriteLine([char]$n)}"') do set "KEY=%%K"

if /i "%KEY%"=="UP" goto KEY_UP
if /i "%KEY%"=="W" goto KEY_UP
if /i "%KEY%"=="DOWN" goto KEY_DOWN
if /i "%KEY%"=="S" goto KEY_DOWN
if /i "%KEY%"=="ENTER" goto EXECUTE_CHOICE

if "%KEY%"=="1" (set "SELECTION=1"&goto EXECUTE_CHOICE)
if "%KEY%"=="2" (set "SELECTION=2"&goto EXECUTE_CHOICE)
if "%KEY%"=="3" (set "SELECTION=3"&goto EXECUTE_CHOICE)
if "%KEY%"=="4" (set "SELECTION=4"&goto EXECUTE_CHOICE)
if "%KEY%"=="5" (set "SELECTION=5"&goto EXECUTE_CHOICE)
if "%KEY%"=="6" (set "SELECTION=6"&goto EXECUTE_CHOICE)
if "%KEY%"=="7" (set "SELECTION=7"&goto EXECUTE_CHOICE)
if "%KEY%"=="8" (set "SELECTION=8"&goto EXECUTE_CHOICE)
if "%KEY%"=="9" (set "SELECTION=9"&goto EXECUTE_CHOICE)

goto MENU


:KEY_UP
if "%SELECTION%"=="1" (set "SELECTION=9") else (set /a SELECTION-=1)
goto MENU

:KEY_DOWN
if "%SELECTION%"=="9" (set "SELECTION=1") else (set /a SELECTION+=1)
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
if "%SELECTION%"=="9" goto EXIT_APP
goto MENU


:: ============================================================
:: LOADING INDICATOR
:: Animated in place, without printing a new line per frame.
:: ============================================================

:LOADING
powershell.exe -NoProfile -Command "$e=[char]27; $frames=@('|','/','-','\'); for($i=0;$i -lt 8;$i++){[Console]::Write($e+'[H'+$e+'[96m  '+$frames[$i %% 4]+' '+$env:LOADING_TEXT+$e+'[0m'); Start-Sleep -Milliseconds 70}"
exit /b


:: ============================================================
:: REQUEST EXECUTION
:: Capture output to a file, then paint the response together.
:: ============================================================

:RUN_GET
call :SHOW_LOADING
darkcom2.exe "%REQUEST_URL%" -X GET > "%OUTFILE%" 2>&1
set "REQUEST_RC=%ERRORLEVEL%"
goto SHOW_RESPONSE

:RUN_GET_SILENT
darkcom2.exe "%REQUEST_URL%" -X GET > "%OUTFILE%" 2>&1
set "REQUEST_RC=%ERRORLEVEL%"
exit /b

:RUN_POST
call :SHOW_LOADING
darkcom2.exe "%REQUEST_URL%" -X POST -d "%JSON_DATA%" > "%OUTFILE%" 2>&1
set "REQUEST_RC=%ERRORLEVEL%"
goto SHOW_RESPONSE

:SHOW_LOADING
set "LOADING_TEXT=CONTACTING GATEWAY NODE..."
call :LOADING
cls
echo.
echo %ESC%[96m  GATEWAY :: ECLIPSE%ESC%[0m
echo %ESC%[90m  ------------------------------------------------------------------------------%ESC%[0m
echo %ESC%[93m  REQUEST%ESC%[0m  %REQUEST_LABEL%
echo %ESC%[90m  Waiting for darkcom2.exe...%ESC%[0m
echo.
exit /b

:SHOW_RESPONSE
cls
echo.
echo %ESC%[96m  GATEWAY :: ECLIPSE%ESC%[0m
echo %ESC%[90m  ------------------------------------------------------------------------------%ESC%[0m
echo %ESC%[93m  RESPONSE%ESC%[0m  %REQUEST_LABEL%
echo %ESC%[90m  ------------------------------------------------------------------------------%ESC%[0m
if exist "%OUTFILE%" (
    powershell.exe -NoProfile -Command "$p=$env:TEMP+'\gateway_response_placeholder.txt'; $p=$env:OUTFILE; if(Test-Path -LiteralPath $p){$t=[IO.File]::ReadAllText($p); [Console]::Write($t)}"
)
echo.
echo %ESC%[90m  ------------------------------------------------------------------------------%ESC%[0m
if not "%REQUEST_RC%"=="0" echo %ESC%[91m  GATEWAY EXIT CODE: %REQUEST_RC%%ESC%[0m
echo.
pause
goto MENU


:: ============================================================
:: APPLICATION STATE
:: ============================================================

:READ_APP_PUB
set "REQUEST_LABEL=READ APPLICATION PUBLIC"
set "REQUEST_URL=%ENDPOINT%?public=true&application=true&write=false"
goto RUN_GET

:WRITE_APP_PUB
set "REQUEST_LABEL=WRITE APPLICATION PUBLIC"
set "REQUEST_URL=%ENDPOINT%?public=true&application=true&write=true"
call :ASK_JSON
if errorlevel 1 goto MENU
goto RUN_POST

:READ_APP_PRIV
set "REQUEST_LABEL=READ APPLICATION PRIVATE"
set "REQUEST_URL=%ENDPOINT%?public=false&application=true&write=false"
goto RUN_GET

:WRITE_APP_PRIV
set "REQUEST_LABEL=WRITE APPLICATION PRIVATE"
set "REQUEST_URL=%ENDPOINT%?public=false&application=true&write=true"
call :ASK_JSON
if errorlevel 1 goto MENU
goto RUN_POST


:: ============================================================
:: SESSION STATE
:: ============================================================

:READ_SES_PUB
set "REQUEST_LABEL=READ SESSION PUBLIC"
set "REQUEST_URL=%ENDPOINT%?public=true&application=false&write=false"
goto RUN_GET

:WRITE_SES_PUB
set "REQUEST_LABEL=WRITE SESSION PUBLIC"
set "REQUEST_URL=%ENDPOINT%?public=true&application=false&write=true"
call :ASK_JSON
if errorlevel 1 goto MENU
goto RUN_POST

:READ_SES_PRIV
set "REQUEST_LABEL=READ SESSION PRIVATE"
set "REQUEST_URL=%ENDPOINT%?public=false&application=false&write=false"
goto RUN_GET

:WRITE_SES_PRIV
set "REQUEST_LABEL=WRITE SESSION PRIVATE"
set "REQUEST_URL=%ENDPOINT%?public=false&application=false&write=true"
call :ASK_JSON
if errorlevel 1 goto MENU
goto RUN_POST


:: ============================================================
:: JSON INPUT
:: ============================================================

:ASK_JSON
cls
echo.
echo %ESC%[96m  GATEWAY :: JSON PAYLOAD%ESC%[0m
echo %ESC%[90m  Enter a single-line JSON payload. Leave blank to cancel.%ESC%[0m
echo.
set "JSON_DATA="
set /p "JSON_DATA=JSON: "
if not defined JSON_DATA exit /b 1
exit /b 0


:: ============================================================
:: EXIT APP
:: ============================================================

:EXIT_APP
if exist "%OUTFILE%" del "%OUTFILE%" >nul 2>&1
<nul set /p "=%ESC%[H%ESC%[2J"
echo.
echo %ESC%[96m  GATEWAY SHUTTING DOWN%ESC%[0m
echo.
endlocal
exit /b 0
