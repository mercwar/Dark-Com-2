<#
.SYNOPSIS
    Automated Git-to-Profile Module Deployment Routine for the DarkCurl Terminal Client Framework.
#>
[CmdletBinding()]
param()

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Calculate active target user PowerShell module storage path directory layout
$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"
if (-not (Test-Path $TargetModuleRoot)) {
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

# 2. Re-verify the existence of the critical locally compiled binary compilation target layers
$CurrentDirectory = Get-Item .
$DllSourcePath = Join-Path $CurrentDirectory.FullName "DarkComHeadless.dll"
$LoaderSourcePath = Join-Path $CurrentDirectory.FullName "WebView2Loader.dll"
$ModuleSourcePath = Join-Path $CurrentDirectory.FullName "DarkCurl.psm1"

if (-not (Test-Path $DllSourcePath) -or -not (Test-Path $ModuleSourcePath)) {
    Write-Error "[ERROR] Core deployment source assets missing. Ensure you compiled the DarkComHeadless.dll before provisioning the script module layout context."
    return
}

# 3. Securely clone, transfer and relocate production runtime binaries directly into the registered system environments
Write-Host "[INFO] Copying core engine execution binaries down into the profile registry directory..." -ForegroundColor Yellow
Copy-Item -Path $DllSourcePath -Destination $TargetModuleRoot -Force
Copy-Item -Path $LoaderSourcePath -Destination $TargetModuleRoot -Force
Copy-Item -Path $ModuleSourcePath -Destination $TargetModuleRoot -Force

# 4. Enforce runtime workspace verification testing procedures to guarantee path boundaries lock clean
Write-Host "[INFO] Registering module variables into current terminal profile parameters..." -ForegroundColor Yellow
Import-Module -Name DarkCurl -Force -ErrorAction Stop

Write-Host "`n[SUCCESS] DarkCurl Engine Pipeline Registered Natively into your PowerShell environment!" -ForegroundColor Green
Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
Write-Host "Usage POST:    dcurl 'https://httpbin.org' 'foo=bar&baz=qux'`n" -ForegroundColor White
