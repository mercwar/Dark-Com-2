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

$TargetModuleRoot = Join-Path $HOME "Documents\DarkComEngine"

if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "[INFO] Creating destination infrastructure layout..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

$FilesToDownload = @(
    @{
        Name     = "DarkComHeadless.dll"
        Url      = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/DarkComHeadless.dll"
        Required = $true
    },
    @{
        Name     = "WebView2Loader.dll"
        Url      = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/WebView2Loader.dll"
        Required = $true
    },
    @{
        Name     = "darkcom2.ps1"
        Url      = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/darkcom2.ps1"
        Required = $true
    },
    @{
        Name     = "darkcom2.bat"
        Url      = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/darkcom2.bat"
        Required = $false
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

$ScriptPath = Join-Path $TargetModuleRoot "darkcom2.ps1"

if (-not (Test-Path $ScriptPath)) {
    Write-Error "[FATAL] darkcom2.ps1 was not installed."
    return
}

Write-Host ""
Write-Host "[SUCCESS] Dark-Com-2 Engine installed successfully to: $TargetModuleRoot" -ForegroundColor Green
Write-Host ""
Write-Host "=====================================================" -ForegroundColor Yellow
Write-Host " IMPORTANT: PowerShell script execution may be disabled." -ForegroundColor Yellow
Write-Host " If running scripts is blocked, execute this command first:" -ForegroundColor White
Write-Host "   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process" -ForegroundColor Cyan
Write-Host ""
Write-Host " To launch the interface, run:" -ForegroundColor White
Write-Host "   & `"$ScriptPath`"" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Yellow
Write-Host ""
