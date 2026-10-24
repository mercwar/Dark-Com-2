[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# Enable ANSI escape processing in modern Windows terminals
$e = [char]27
$ESC = "$e"

Clear-Host
Write-Host "$ESC[96m=====================================================================$ESC[0m"
Write-Host "$ESC[96m Dark-Com-2 Engine Global PowerShell Installer                      $ESC[0m"
Write-Host "$ESC[96m=====================================================================$ESC[0m"
Write-Host ""

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
} catch {}

$TargetModuleRoot = Join-Path $HOME "Documents\DarkComEngine"

if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "$ESC[33m[INFO] Creating destination infrastructure layout...$ESC[0m"
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

Set-Location $TargetModuleRoot
Write-Host "$ESC[90m[INFO] Working Directory set to: $(Get-Location)$ESC[0m"
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

Write-Host "$ESC[94m[INFO] Fetching core engine deployment layers from GitHub...$ESC[0m"
Write-Host ""

# Helper function to paint a modern flat block progress bar
function Update-ProgressBar {
    param (
        [int]$Current,
        [int]$Total,
        [string]$FileName
    )
    $Percent = [Math]::Floor(($Current / $Total) * 100)
    $Width = 30
    $FilledWidth = [Math]::Floor(($Percent / 100) * $Width)
    $UnfilledWidth = $Width - $FilledWidth
    
    # Character maps: Solid block (█) and light shade track (░)
    $FilledBlock = [char]9608
    $UnfilledBlock = [char]9617
    
    $Bar = ("$FilledBlock" * $FilledWidth) + ("$UnfilledBlock" * $UnfilledWidth)
    
    # Modern terminal coloring: Slate blue track, bright cyan filled segment
    $PrintStr = "$ESC[G$ESC[90m[$ESC[96m$Bar$ESC[90m] $ESC[97m$Percent% $ESC[90m| Deploying: $ESC[38;5;111m$FileName$ESC[0m"
    [Console]::Write($PrintStr)
}

$FailedFiles = @()
$TotalFiles = $FilesToDownload.Count
$CurrentIndex = 0

# Initial baseline render of the track
Update-ProgressBar -Current 0 -Total $TotalFiles -FileName "Initializing Pipeline..."

foreach ($File in $FilesToDownload) {
    $Destination = Join-Path $TargetModuleRoot $File.Name
    $CurrentIndex++

    try {
        # Smoothly advance the bar frame context for the asset
        Update-ProgressBar -Current $CurrentIndex -Total $TotalFiles -FileName $File.Name

        Invoke-WebRequest `
            -Uri $File.Url `
            -OutFile $Destination `
            -UseBasicParsing `
            -TimeoutSec 30 `
            -ErrorAction Stop

        if (-not (Test-Path $Destination)) {
            throw "File creation anomaly detected."
        }

        $FileSize = (Get-Item $Destination).Length
        if ($FileSize -le 0) {
            throw "Target structural asset size validation returned 0 bytes."
        }
    }
    catch {
        if ($File.Required) {
            $FailedFiles += $File.Name
        }
    }
}

# Finalize the progress line to 100% cleanly
Update-ProgressBar -Current $TotalFiles -Total $TotalFiles -FileName "Deployment Layout Ready"
Write-Host "`n"

if ($FailedFiles.Count -gt 0) {
    Write-Error "$ESC[91m[FATAL] Missing required core architecture files: $($FailedFiles -join ', ')$ESC[0m"
    return
}

$ScriptPath = ".\darkcom2.ps1"
$BatPath = ".\darkcom2.bat"

Write-Host "[SUCCESS] Dark-Com-2 Engine infrastructure deployed successfully." -ForegroundColor Green
Write-Host ""

# Overhauled decision branch loop context
$response = Read-Host "Would you like to run Eclips now? Press Y to run darkcom2.bat, press N to run darkcom2.ps1"

if ($response -match '^[Yy]') {
    Write-Host "$ESC[93m[INFO] Invoking state broker interactive console menu...$ESC[0m"
    if (Test-Path $BatPath) {
        & cmd.exe /c "$BatPath"
    } else {
        Write-Error "$ESC[91m[ERROR] Batch routing asset is missing or inaccessible.$ESC[0m"
    }
} 
elseif ($response -match '^[Nn]') {
    Write-Host "$ESC[90m[INFO] Bypassing execution policies for headless Runspace Broker thread...$ESC[0m"
    if (Test-Path $ScriptPath) {
        $ProcessArgs = "-NoProfile -ExecutionPolicy Bypass -File `"$ScriptPath`""
        Start-Process -FilePath "powershell.exe" -ArgumentList $ProcessArgs -NoNewWindow -Wait
    } else {
        Write-Error "$ESC[91m[ERROR] Native PowerShell backend execution script is missing.$ESC[0m"
    }
} 
else {
    Write-Host "$ESC[90m[INFO] Installer sequence exiting. Launch your chosen protocol manually inside $TargetModuleRoot$ESC[0m"
}
