@echo off
setlocal EnableDelayedExpansion

:: ============================================================================
:: ECLIPSE STATE BROKER CONSOLE (DARKCOM2 ENGINE BRIDGE)
:: ============================================================================
title ECLIPSE STATE BROKER
color 0B
cd /d "%~dp0"

:: Verified working path with correct 'repositories' spelling and http:// protocol
set "ENDPOINT=http://localhost:8080/CL2/repositories/mercwar/Dark-Com-2/eclipse.php"
set "OUTFILE=%TEMP%\eclipse_response_%RANDOM%.txt"

:: Set default wizard states on load
set "SELECTED_APP=ECLIPSE"
set "SELECTED_SCOPE=private"
set "SELECTED_METHOD=set"
set "SELECTED_TYPE=json"

:MAIN_MENU
cls
echo ============================================================================
echo                      ECLIPSE PROTOCOL :: STATE BROKER
echo ============================================================================
echo   Endpoint: %ENDPOINT%
echo ----------------------------------------------------------------------------
echo.
echo [1] Select Application     (Current: !SELECTED_APP!)
echo [2] Select Security        (Current: !SELECTED_SCOPE!)
echo [3] Select Method          (Current: !SELECTED_METHOD!)
echo [4] Select Data Type       (Current: !SELECTED_TYPE!)
echo [5] Execute Request
echo [6] Exit
echo.
set "CHOICE="
set /p "CHOICE=Select Option [1-6]: "

if "%CHOICE%"=="1" goto WIZ_APP
if "%CHOICE%"=="2" goto WIZ_SECURITY
if "%CHOICE%"=="3" goto WIZ_METHOD
if "%CHOICE%"=="4" goto WIZ_TYPE
if "%CHOICE%"=="5" goto WIZ_EXECUTE
if "%CHOICE%"=="6" goto EXIT_APP
goto MAIN_MENU


:: ============================================================
:: 1. SELECT APPLICATION
:: ============================================================
:WIZ_APP
cls
echo ============================================================================
echo [ STEP 1 ] SELECT APPLICATION
echo ============================================================================
echo [1] ECLIPSE
echo [2] CUSTOM
echo.
set "APP_OPT="
set /p "APP_OPT=Choice [1-2]: "
if "%APP_OPT%"=="1" (
    set "SELECTED_APP=ECLIPSE"
) else if "%APP_OPT%"=="2" (
    set /p "SELECTED_APP=Enter Custom App Name: "
    if not defined SELECTED_APP set "SELECTED_APP=CUSTOM"
)
goto MAIN_MENU


:: ============================================================
:: 2. SELECT SECURITY
:: ============================================================
:WIZ_SECURITY
cls
echo ============================================================================
echo [ STEP 2 ] SELECT SECURITY SCOPE
echo ============================================================================
echo [1] public
echo [2] private
echo.
set "SEC_OPT="
set /p "SEC_OPT=Choice [1-2]: "
if "%SEC_OPT%"=="1" (
    set "SELECTED_SCOPE=public"
) else if "%SEC_OPT%"=="2" (
    set "SELECTED_SCOPE=private"
)
goto MAIN_MENU


:: ============================================================
:: 3. SELECT METHOD
:: ============================================================
:WIZ_METHOD
cls
echo ============================================================================
echo [ STEP 3 ] SELECT METHOD
echo ============================================================
echo [1] set
echo [2] get
echo.
set "MTH_OPT="
set /p "MTH_OPT=Choice [1-2]: "
if "%MTH_OPT%"=="1" (
    set "SELECTED_METHOD=set"
) else if "%MTH_OPT%"=="2" (
    set "SELECTED_METHOD=get"
)
goto MAIN_MENU


:: ============================================================
:: 4. SELECT TYPE
:: ============================================================
:WIZ_TYPE
cls
echo ============================================================================
echo [ STEP 4 ] SELECT DATA TYPE
echo ============================================================
echo [1] json
echo [2] text
echo.
set "TYP_OPT="
set /p "TYP_OPT=Choice [1-2]: "
if "%TYP_OPT%"=="1" (
    set "SELECTED_TYPE=json"
) else if "%TYP_OPT%"=="2" (
    set "SELECTED_TYPE=text"
)
goto MAIN_MENU


:: ============================================================
:: 5. EXECUTE REQUEST
:: ============================================================
:WIZ_EXECUTE
cls
echo ============================================================================
echo [ STEP 5 ] EXECUTE REQUEST
echo ============================================================================
echo   Application : %SELECTED_APP%
echo   Security    : %SELECTED_SCOPE%
echo   Method      : %SELECTED_METHOD%
echo   Type        : %SELECTED_TYPE%
echo ----------------------------------------------------------------------------
echo.

if /i "%SELECTED_METHOD%"=="set" (
    echo Enter payload data to save into session container:
    set "USER_PAYLOAD="
    set /p "USER_PAYLOAD=Payload: "
    if not defined USER_PAYLOAD goto MAIN_MENU

    set "REQUEST_LABEL=SET [%SELECTED_APP% / %SELECTED_SCOPE% / %SELECTED_TYPE%]"
    
    for /f "delims=" %%J in ('powershell.exe -NoProfile -Command "$obj = @{ action = 'set'; app = '%SELECTED_APP%'; type = '%SELECTED_TYPE%'; scope = '%SELECTED_SCOPE%'; payload = $env:USER_PAYLOAD }; $obj | ConvertTo-Json -Compress -Depth 5"') do set "JSON_DATA=%%J"

) else (
    set "REQUEST_LABEL=GET [%SELECTED_APP% / %SELECTED_SCOPE% / %SELECTED_TYPE%]"
    
    for /f "delims=" %%J in ('powershell.exe -NoProfile -Command "$obj = @{ action = 'get'; app = '%SELECTED_APP%'; node_address = @('%SELECTED_APP%', '%SELECTED_SCOPE%', '%SELECTED_TYPE%') }; $obj | ConvertTo-Json -Compress -Depth 5"') do set "JSON_DATA=%%J"
)

echo.
echo Transmitting to server via darkcom2.exe...
darkcom2.exe "%ENDPOINT%" -X POST -d "%JSON_DATA%" > "%OUTFILE%" 2>&1
set "REQUEST_RC=%ERRORLEVEL%"

cls
echo ============================================================================
echo RESPONSE :: %REQUEST_LABEL%
echo ============================================================================
if exist "%OUTFILE%" type "%OUTFILE%"
echo.
echo ============================================================================
if not "%REQUEST_RC%"=="0" echo GATEWAY EXIT CODE: %REQUEST_RC%
echo.
pause
goto MAIN_MENU


:: ============================================================
:: EXIT
:: ============================================================
:EXIT_APP
if exist "%OUTFILE%" del "%OUTFILE%" >nul 2>&1
cls
echo Exiting Eclipse Gateway...
endlocal
exit /b 0