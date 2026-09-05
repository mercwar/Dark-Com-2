@echo off
title Listing All COM Objects
echo Fetching unique COM object ProgIDs from the registry...
echo Please wait, this may take a moment...
echo.

powershell -NoProfile -Command "Get-ChildItem -Path 'REGISTRY::HKey_Classes_Root\clsid\*\progid' | ForEach-Object { if ($_.Name -like '*\ProgID') { $_.GetValue('') } } | Sort-Object -Unique"

echo.
echo Done!
pause
