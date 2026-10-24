[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

try {
    [Net.ServicePointManager]::SecurityProtocol = `
        [Net.SecurityProtocolType]::Tls12
} catch {}

$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"

if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "[INFO] Creating destination module infrastructure layout..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

$FilesToDownload = @(
    @{
        Name = "DarkComHeadless.dll"
        Url  = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/DarkComHeadless.dll"
        Required = $true
    },
    @{
        Name = "WebView2Loader.dll"
        Url  = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/WebView2Loader.dll"
        Required = $true
    },
    @{
        Name = "DarkCurl.psm1"
        Url  = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/DarkCurl.psm1"
        Required = $true
    }
)

Write-Host "[INFO] Fetching core engine deployment layers from GitHub..." -ForegroundColor Yellow

$FailedFiles = @()

foreach ($File in $FilesToDownload) {

    $Destination = Join-Path $TargetModuleRoot $File.Name

    try {
        Write-Host " -> Downloading: $($File.Name)" -ForegroundColor Gray

        Invoke-WebRequest `
            -Uri $File.Url `
            -OutFile $Destination `
            -UseBasicParsing `
            -TimeoutSec 30 `
            -ErrorAction Stop

        if (-not (Test-Path $Destination)) {
            throw "File was not created."
        }

        $FileSize = (Get-Item $Destination).Length

        if ($FileSize -le 0) {
            throw "Downloaded file is empty."
        }

        Write-Host "    [OK] $($File.Name) ($FileSize bytes)" -ForegroundColor Green
    }
    catch {
        Write-Warning "[WARN] Failed to download $($File.Name)"
        Write-Warning $_.Exception.Message

        if ($File.Required) {
            $FailedFiles += $File.Name
        }
    }
}

if ($FailedFiles.Count -gt 0) {
    Write-Error "[FATAL] Missing required files: $($FailedFiles -join ', ')"
    return
}

$ModulePath = Join-Path $TargetModuleRoot "DarkCurl.psm1"

if (-not (Test-Path $ModulePath)) {
    Write-Error "[FATAL] DarkCurl.psm1 was not installed."
    return
}

Write-Host "[INFO] Registering module variables into current terminal profile parameters..." -ForegroundColor Yellow

try {
    Import-Module $ModulePath -Force -ErrorAction Stop

    Write-Host ""
    Write-Host "[SUCCESS] DarkCurl Engine Pipeline Registered Natively into your PowerShell environment!" -ForegroundColor Green
    Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
    Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
    Write-Host "Usage POST:    dcurl 'https://httpbin.org/post' 'foo=bar&baz=qux'" -ForegroundColor White
    Write-Host ""
}
catch {
    Write-Error "[FATAL] Failed to initialize module path context layout natively: $($_.Exception.Message)"
}
