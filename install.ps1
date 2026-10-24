[CmdletBinding()]
param()

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Determine the actual execution context source path
if ($MyInvocation.ScriptSourceLocation) {
    # File is running locally as a script
    $SourcePath = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
} else {
    # File is running via iex/in-memory (default fallback to the parent caller context or current path)
    $SourcePath = (Get-Location).Path
}

# Fix system32 path hijacking if running in an elevated prompt via raw iex
if ($SourcePath -eq "$env:SystemRoot\system32") {
    Write-Warning "[WARN] Execution path resolved to system32. Attempting to locate assets in user space..."
    $SourcePath = $PSScriptRoot
    if (-not $SourcePath) { $SourcePath = $HOME }
}

$DllSourcePath    = Join-Path $SourcePath "DarkComHeadless.dll"
$LoaderSourcePath = Join-Path $SourcePath "WebView2Loader.dll"
$ModuleSourcePath = Join-Path $SourcePath "DarkCurl.psm1"

# 2. Establish target module container repository
$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"
if (-not (Test-Path $TargetModuleRoot)) {
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

# 3. Verify component existence before moving
if (-not (Test-Path $DllSourcePath) -or -not (Test-Path $ModuleSourcePath)) {
    Write-Error "[ERROR] Core deployment source assets missing."
    Write-Error "Looked in: $SourcePath"
    Write-Error "Ensure 'DarkComHeadless.dll' and 'DarkCurl.psm1' are placed inside that directory before running."
    return
}

# 4. Perform localized migration to user environment paths
Write-Host "[INFO] Copying core engine execution binaries down into the profile registry directory..." -ForegroundColor Yellow
Copy-Item -Path $DllSourcePath -Destination $TargetModuleRoot -Force
Copy-Item -Path $ModuleSourcePath -Destination $TargetModuleRoot -Force

if (Test-Path $LoaderSourcePath) {
    Copy-Item -Path $LoaderSourcePath -Destination $TargetModuleRoot -Force
}

# 5. Native session integration verification
Write-Host "[INFO] Registering module variables into current terminal profile parameters..." -ForegroundColor Yellow
try {
    Import-Module -Name $TargetModuleRoot -Force -ErrorAction Stop
    Write-Host "`n[SUCCESS] DarkCurl Engine Pipeline Registered Natively into your PowerShell environment!" -ForegroundColor Green
    Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
    Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
}
catch {
    Write-Error "[FATAL] Failed to initialize module path context layout natively: $_"
}
