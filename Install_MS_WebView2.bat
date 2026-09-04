@echo off
echo ==========================================
echo AVIS - WebView2 Installer Script (AIFVS)
echo ==========================================

:: Force execution to lock onto the directory where this .bat file actually lives
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo Target folder set to: %SCRIPT_DIR%

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$ProgressPreference = 'SilentlyContinue';" ^
    "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;" ^
    "$pkgDir = Join-Path '%SCRIPT_DIR:\=\%' 'packages\Microsoft.Web.WebView2';" ^
    "Write-Host 'Downloading NuGet package...';" ^
    "Invoke-WebRequest -Uri 'https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2' -OutFile 'webview2.nupkg' -UseBasicParsing;" ^
    "Write-Host 'Extracting package...';" ^
    "if (Test-Path 'webview2.zip') { Remove-Item 'webview2.zip' -Force };" ^
    "Rename-Item 'webview2.nupkg' 'webview2.zip' -Force;" ^
    "Expand-Archive -Path 'webview2.zip' -DestinationPath $pkgDir -Force;" ^
    "Remove-Item 'webview2.zip' -Force;" ^
    "Write-Host 'WebView2 installed successfully to:' $pkgDir"

pause
