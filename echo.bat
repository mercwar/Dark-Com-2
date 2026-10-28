@echo off
setlocal enabledelayedexpansion

set "TARGET_URL=http://mercwar01.byethost3.com/darkcom2/echo.php"
set "MIDI_URL=https://github.com/mercwar/dc2url/raw/refs/heads/main/sound/transformers.mid"
set "TEST_MIDI=%TEMP%\darkcom2_test.mid"

echo ============================================================
echo  DARK-COM-2 :: FULL CURL-COMPATIBLE VERIFICATION SUITE
echo ============================================================
echo.

if exist "%TEST_MIDI%" del /f /q "%TEST_MIDI%"

echo [TEST 1] URL First (Standard Curl Style)
echo ------------------------------------------------------------
dc2url "%TARGET_URL%" -X POST -d "{\"action\":\"test_url_first\"}" -v

echo.
echo ============================================================
echo [TEST 2] Flags First (Alternative Curl Style)
echo ------------------------------------------------------------
dc2url -X POST -d "{\"action\":\"test_flags_first\"}" "%TARGET_URL%" --verbose

echo.
echo ============================================================
echo [TEST 3] Long Flags Style
echo ------------------------------------------------------------
dc2url --request POST --data "{\"action\":\"test_long_flags\"}" "%TARGET_URL%" --verbose

echo.
echo ============================================================
echo  SEQUENTIAL VERIFICATION: PERSISTENCE TRACKING BLOCK
echo ============================================================
echo.
echo Iteration #1: Sending payload...
dc2url --request POST --data "{\"iteration\":1}" "%TARGET_URL%" -v
echo.

echo Iteration #2: Testing value increment updates...
dc2url --request POST --data "{\"iteration\":2}" "%TARGET_URL%" -v
echo.

echo ============================================================
echo  BINARY DOWNLOAD TEST: BASE64 BRIDGE VERIFICATION
echo ============================================================
echo Downloading binary MIDI asset via dc2url -o flag...
dc2url -o "%TEST_MIDI%" "%MIDI_URL%" -v

echo.
if exist "%TEST_MIDI%" (
    for %%I in ("%TEST_MIDI%") do set "FILESIZE=%%~zI"
    echo [SUCCESS] Binary download completed successfully!
    echo [FILE] Target: %TEST_MIDI%
    echo [SIZE] Bytes transferred: !FILESIZE! bytes
    
    if !FILESIZE! GTR 1000 (
        echo [VERIFICATION] File size is valid. Launching audio playback test...
        start wmplayer "%TEST_MIDI%"
    ) else (
        echo [WARNING] File size is suspiciously small. Verify base64 decoding buffer.
    )
) else (
    echo [ERROR] Binary download failed. Output file was not created.
)

echo.
echo ============================================================
echo  ALL API DIAGNOSTICS AND BINARY TESTS COMPLETED
echo ============================================================
pause