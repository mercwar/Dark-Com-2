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

Write-Host "$ESC[96m[INFO] Fetching core engine deployment layers from GitHub...$ESC[0m"
Write-Host ""

$FailedFiles = @()
$TotalFiles = $FilesToDownload.Count
$CurrentIndex = 0

foreach ($File in $FilesToDownload) {
    $CurrentIndex++
    $Destination = Join-Path $TargetModuleRoot $File.Name

    # Render modern smooth progress bar using ANSI colors without flickering boxes
    $Width = 25
    $Percent = [Math]::Floor(($CurrentIndex / $TotalFiles) * 100)
    $Completed = [Math]::Floor(($Width * $Percent) / 100)
    $Remaining = $Width - $Completed
    
    $BarFilled = "$ESC[96m" + ('█' * $Completed) + "$ESC[0m"
    $BarEmpty = "$ESC[90m" + ('░' * $Remaining) + "$ESC[0m"
    
    [Console]::Write("`r  [$BarFilled$BarEmpty] $ESC[93m$Percent%%$ESC[0m ($CurrentIndex/$TotalFiles) Downloading: $ESC[97m$($File.Name)$ESC[0m                    ")

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
        [Console]::Write("`n")
        Write-Warning "[WARN] Failed to download $($File.Name): $($_.Exception.Message)"
        if ($File.Required) {
            $FailedFiles += $File.Name
        }
    }
}

Write-Host "`n"
Write-Host "$ESC[90m=====================================================================$ESC[0m"

if ($FailedFiles.Count -gt 0) {
    Write-Error "[FATAL] Missing required files: $($FailedFiles -join ', ')"
    return
}

$ScriptPath = ".\darkcom2.ps1"
$BatPath = ".\darkcom2.bat"

Write-Host "$ESC[92m[SUCCESS] Dark-Com-2 Engine installed successfully!$ESC[0m"
Write-Host "$ESC[90mLocation: $(Get-Location)$ESC[0m"
Write-Host ""

$response = Read-Host "Would you like to run darkcom2.ps1 now? (Y/N)"

if ($response -match '^[Yy]') {
    Write-Host "$ESC[33m[INFO] Attempting to launch darkcom2.ps1 from install directory...$ESC[0m"
    try {
        & powershell.exe -ExecutionPolicy Bypass -File "$ScriptPath"
    }
    catch {
        Write-Warning "[WARN] PowerShell script execution failed/blocked. Falling back to batch file..."
        & cmd.exe /c "$BatPath"
    }
} else {
    Write-Host "$ESC[33m[INFO] Launching darkcom2.bat from install directory...$ESC[0m"
    & cmd.exe /c "$BatPath"
}
