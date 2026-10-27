@echo off
setlocal enabledelayedexpansion

:: Dynamically capture the directory where this batch file lives
set "ROOT_DIR=%~dp0"
 set "TARGET_URL=http://mercwar01.byethost3.com/darkcom2/echo.php"
rem set "TARGET_URL=http://localhost:8080/CL2/repositories/mercwar/Dark-Com-2/echo.php"

echo ============================================================
echo  DARK-COM-2 :: CURL-COMPATIBLE VERIFICATION SUITE
echo ============================================================
echo.

echo [TEST 1] URL First (Standard Curl Style)
echo ------------------------------------------------------------
if exist "!ROOT_DIR!dc2url.exe" (
    "!ROOT_DIR!dc2url.exe" "%TARGET_URL%" -X POST -d "{\"action\":\"test_url_first\"}" -v
) else (
    echo [ERROR] darkcom2.exe not found in "!ROOT_DIR!"
)

echo.
echo ============================================================
echo.

echo [TEST 2] Flags First (Alternative Curl Style)
echo ------------------------------------------------------------
if exist "!ROOT_DIR!dc2url.exe" (
    "!ROOT_DIR!dc2url.exe" -X POST -d "{\"action\":\"test_flags_first\"}" "%TARGET_URL%" --verbose
)

echo.
echo ============================================================
echo.

echo [TEST 3] Long Flags Style
echo ------------------------------------------------------------
if exist "!ROOT_DIR!dc2url.exe" (
    "!ROOT_DIR!dc2url.exe" --request POST --data "{\"action\":\"test_long_flags\"}" "%TARGET_URL%" --verbose
)

echo.
echo ============================================================
echo  SEQUENTIAL VERIFICATION: PERSISTENCE TRACKING BLOCK
echo ============================================================
echo.
echo Running back-to-back iterations to verify system variable increments...
echo.

echo Iteration #1: Sending payload...
if exist "!ROOT_DIR!dc2url.exe" (
    "!ROOT_DIR!dc2url.exe" --request POST --data "{\"iteration\":1}" "%TARGET_URL%" -v
)
echo.

echo Iteration #2: Testing value increment updates...
if exist "!ROOT_DIR!dc2url.exe" (
    "!ROOT_DIR!dc2url.exe" --request POST --data "{\"iteration\":2}" "%TARGET_URL%" -v
)
echo.

echo ============================================================
echo  ALL DIAGNOSTIC AND PERSISTENCE TESTS COMPLETED
echo ============================================================
pause
