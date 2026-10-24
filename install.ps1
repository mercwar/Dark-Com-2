[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# Clear screen and set up clean header
Clear-Host
Write-Host "=====================================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================================" -ForegroundColor Cyan
Write-Host ""

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
} catch {}

$TargetModuleRoot = Join-Path $HOME "Documents\DarkComEngine"

if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "[INFO] Creating destination infrastructure layout..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

# Change to the installation directory FIRST before downloading/running anything
Set-Location $TargetModuleRoot
Write-Host "[INFO] Working Directory set to: $(Get-Location)" -ForegroundColor DarkGray
Write-Host ""

$FilesToDownload = @(
    @{
        Name     = "darkcom2.exe"
        Url      = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/darkcom2.exe"
        Required = $true
    },
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
        Required = $true
    }
)

Write-Host "[INFO] Fetching core engine deployment layers from GitHub..." -ForegroundColor Cyan
Write-Host ""

$FailedFiles = @()
$TotalFiles = $FilesToDownload.Count
$CurrentIndex = 0

foreach ($File in $FilesToDownload) {
    $CurrentIndex++
    $Destination = Join-Path $TargetModuleRoot $File.Name

    # Render custom sleek progress bar
    $Width = 25
    $Percent = [Math]::Floor(($CurrentIndex / $TotalFiles) * 100)
    $Completed = [Math]::Floor(($Width * $Percent) / 100)
    $Remaining = $Width - $Completed
    $Bar = ('█' * $Completed) + ('░' * $Remaining)
    
    Write-Host -NoNewline "`r  [$Bar] $Percent% ($CurrentIndex/$TotalFiles) Downloading: $($File.Name)...                    "

    try {
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
    }
    catch {
        Write-Host ""
        Write-Warning "[WARN] Failed to download $($File.Name): $($_.Exception.Message)"
        if ($File.Required) {
            $FailedFiles += $File.Name
        }
    }
}

# Finish progress line neatly
Write-Host "`n"
Write-Host "=====================================================================" -ForegroundColor DarkGray

if ($FailedFiles.Count -gt 0) {
    Write-Error "[FATAL] Missing required files: $($FailedFiles -join ', ')"
    return
}

$ScriptPath = ".\darkcom2.ps1"
$BatPath = ".\darkcom2.bat"

Write-Host "[SUCCESS] Dark-Com-2 Engine installed successfully!" -ForegroundColor Green
Write-Host "Location: $(Get-Location)" -ForegroundColor DarkGray
Write-Host ""

$response = Read-Host "Would you like to run darkcom2.ps1 now? (Y/N)"

if ($response -match '^[Yy]') {
    Write-Host "[INFO] Attempting to launch darkcom2.ps1 from install directory..." -ForegroundColor Yellow
    try {
        & powershell.exe -ExecutionPolicy Bypass -File "$ScriptPath"
    }
    catch {
        Write-Warning "[WARN] PowerShell script execution failed/blocked. Falling back to batch file..."
        & cmd.exe /c "$BatPath"
    }
} else {
    Write-Host "[INFO] Launching darkcom2.bat from install directory..." -ForegroundColor Yellow
    & cmd.exe /c "$BatPath"
}
