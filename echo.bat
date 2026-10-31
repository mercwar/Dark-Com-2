@echo off
setlocal EnableExtensions DisableDelayedExpansion
title DARK-COM-2 :: DC2URL INTERNET TEST SUITE

cd /d "%~dp0"

set "EXE=%~dp0dc2url.exe"
set "TESTROOT=%~dp0DC2URL-TEST-RESULTS"
set "LOGDIR=%TESTROOT%\logs"
set "REPORT=%TESTROOT%\TEST-REPORT.txt"
set /a TOTAL=0
set /a PASS=0
set /a FAIL=0

if not exist "%EXE%" (
    echo.
    echo [FATAL] dc2url.exe was not found:
    echo         "%EXE%"
    echo.
    echo Place this batch file beside dc2url.exe.
    pause
    exit /b 2
)

if not exist "%TESTROOT%" mkdir "%TESTROOT%"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

if not exist "%LOGDIR%" (
    echo [FATAL] Cannot create test output directory.
    pause
    exit /b 2
)

> "%REPORT%" echo DARK-COM-2 :: DC2URL INTERNET TEST REPORT
>>"%REPORT%" echo Date: %DATE% %TIME%
>>"%REPORT%" echo Executable: %EXE%
>>"%REPORT%" echo ============================================================

echo.
echo ============================================================
echo    DARK-COM-2 :: DC2URL v2.3
echo    INTERNET INTEGRATION TEST SUITE
echo ============================================================
echo.
echo Executable: "%EXE%"
echo Logs:       "%LOGDIR%"
echo Report:     "%REPORT%"
echo.
echo Testing public internet services.
echo Requests are sequential and intentionally limited.
echo.

"%EXE%" --help > "%LOGDIR%\00-help.txt" 2>&1
echo [INFO] Help output saved.

REM ============================================================
REM TEST 01: HTTPS GET
REM ============================================================

echo [01/14] HTTPS GET connectivity...
"%EXE%" -X GET "https://postman-echo.com/get?test=dc2url&suite=basic" > "%LOGDIR%\01-get.txt" 2>&1
call :record "01 - HTTPS GET" %ERRORLEVEL%

REM ============================================================
REM TEST 02: SECOND INDEPENDENT HTTPS SERVER
REM ============================================================

echo [02/14] HTTPBin GET...
"%EXE%" -X GET "https://httpbin.org/get?test=dc2url" > "%LOGDIR%\02-httpbin-get.txt" 2>&1
call :record "02 - HTTPBin GET" %ERRORLEVEL%

REM ============================================================
REM TEST 03: CUSTOM USER AGENT
REM ============================================================

echo [03/14] Custom user-agent...
"%EXE%" -X GET -A "DarkCom2-DC2URL-Test/2.3" "https://postman-echo.com/get" > "%LOGDIR%\03-user-agent.txt" 2>&1
call :record "03 - Custom user-agent" %ERRORLEVEL%

REM ============================================================
REM TEST 04: CUSTOM HEADER
REM ============================================================

echo [04/14] Custom HTTP header...
"%EXE%" -X GET -H "X-DC2URL-Test: MERCWAR-TEST-23" "https://postman-echo.com/headers" > "%LOGDIR%\04-header.txt" 2>&1
call :record "04 - Custom header" %ERRORLEVEL%

REM ============================================================
REM TEST 05: POST JSON
REM ============================================================

echo [05/14] POST JSON payload...
"%EXE%" -X POST -H "Content-Type: application/json" -d "{\"project\":\"Dark-Com-2\",\"client\":\"dc2url\",\"test\":5}" "https://postman-echo.com/post" > "%LOGDIR%\05-post-json.txt" 2>&1
call :record "05 - POST JSON" %ERRORLEVEL%

REM ============================================================
REM TEST 06: PUT
REM ============================================================

echo [06/14] PUT request...
"%EXE%" -X PUT -H "Content-Type: text/plain" -d "DC2URL PUT INTEGRATION TEST" "https://postman-echo.com/put" > "%LOGDIR%\06-put.txt" 2>&1
call :record "06 - PUT request" %ERRORLEVEL%

REM ============================================================
REM TEST 07: DELETE
REM Safe: Postman Echo does not delete a real user resource.
REM ============================================================

echo [07/14] DELETE method...
"%EXE%" -X DELETE "https://postman-echo.com/delete" > "%LOGDIR%\07-delete.txt" 2>&1
call :record "07 - DELETE method" %ERRORLEVEL%

REM ============================================================
REM TEST 08: HEAD
REM ============================================================

echo [08/14] HEAD request...
"%EXE%" -I "https://postman-echo.com/get" > "%LOGDIR%\08-head.txt" 2>&1
call :record "08 - HEAD request" %ERRORLEVEL%

REM ============================================================
REM TEST 09: DOWNLOAD RESPONSE TO FILE
REM ============================================================

echo [09/14] Download response to disk...
if exist "%TESTROOT%\download.json" del /q "%TESTROOT%\download.json" >nul 2>&1

"%EXE%" -X GET -o "%TESTROOT%\download.json" "https://postman-echo.com/get?download=dc2url" > "%LOGDIR%\09-download.txt" 2>&1
call :record "09 - Download to file" %ERRORLEVEL%

if exist "%TESTROOT%\download.json" (
    for %%F in ("%TESTROOT%\download.json") do (
        if %%~zF GTR 0 (
            echo        Download file exists and is non-empty.
            >>"%REPORT%" echo        Download file exists and is non-empty.
        ) else (
            echo        WARNING: Download file is empty.
            >>"%REPORT%" echo        WARNING: Download file is empty.
        )
    )
) else (
    echo        WARNING: Download file was not created.
    >>"%REPORT%" echo        WARNING: Download file was not created.
)

REM ============================================================
REM TEST 10: REDIRECT
REM This tests behavior; whether redirects are followed is
REM determined by the implementation of dc2url.
REM ============================================================

echo [10/14] Redirect handling...
"%EXE%" -X GET "https://httpbin.org/redirect/2" > "%LOGDIR%\10-redirect.txt" 2>&1
call :record "10 - Redirect handling" %ERRORLEVEL%

REM ============================================================
REM TEST 11: HTTP ERROR STATUS
REM HTTP 404 is intentional. Inspect the response for status.
REM ============================================================

echo [11/14] HTTP 404 handling...
"%EXE%" -X GET "https://httpbin.org/status/404" > "%LOGDIR%\11-http-404.txt" 2>&1
call :record "11 - HTTP 404 response" %ERRORLEVEL%

REM ============================================================
REM TEST 12: VERBOSE DIAGNOSTICS
REM ============================================================

echo [12/14] Verbose diagnostics...
"%EXE%" -v -X GET "https://postman-echo.com/get?verbose=1" > "%LOGDIR%\12-verbose.txt" 2>&1
call :record "12 - Verbose diagnostics" %ERRORLEVEL%

REM ============================================================
REM TEST 13: NO JAVASCRIPT MODE
REM ============================================================

echo [13/14] No-JS binary mode...
"%EXE%" --nojs -X GET "https://postman-echo.com/get?mode=nojs" > "%LOGDIR%\13-nojs.txt" 2>&1
call :record "13 - No-JS mode" %ERRORLEVEL%

REM ============================================================
REM TEST 14: LOCAL DLL MODE
REM ============================================================

echo [14/14] Local DLL mode...
"%EXE%" -l -X GET "https://postman-echo.com/get?mode=local" > "%LOGDIR%\14-local-dll.txt" 2>&1
call :record "14 - Local DLL mode" %ERRORLEVEL%

REM ============================================================
REM FINAL REPORT
REM ============================================================

echo.
echo ============================================================
echo                   TEST SUMMARY
echo ============================================================
echo Total:  %TOTAL%
echo Passed: %PASS%
echo Failed: %FAIL%
echo.
echo Report: "%REPORT%"
echo Logs:   "%LOGDIR%"
echo ============================================================

>>"%REPORT%" echo.
>>"%REPORT%" echo ============================================================
>>"%REPORT%" echo SUMMARY
>>"%REPORT%" echo Total: %TOTAL%
>>"%REPORT%" echo Passed: %PASS%
>>"%REPORT%" echo Failed: %FAIL%
>>"%REPORT%" echo ============================================================

if "%FAIL%"=="0" (
    echo.
    echo [RESULT] ALL COMMANDS RETURNED SUCCESS.
    echo         Review response contents to confirm correctness.
    >>"%REPORT%" echo Result: All commands returned success; response validation remains necessary.
) else (
    echo.
    echo [RESULT] SOME COMMANDS RETURNED NONZERO EXIT CODES.
    echo         Open the individual logs to investigate.
    >>"%REPORT%" echo Result: Some commands returned nonzero exit codes.
)

echo.
pause
exit /b 0

REM ============================================================
REM RECORD TEST RESULT
REM ============================================================

:record
set /a TOTAL+=1

if "%~2"=="0" (
    set /a PASS+=1
    echo        [EXIT 0] %~1
    >>"%REPORT%" echo [EXIT 0] %~1
) else (
    set /a FAIL+=1
    echo        [EXIT %~2] %~1
    >>"%REPORT%" echo [EXIT %~2] %~1
)
exit /b