[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Enable ANSI escape processing
$e = [char]27
$ESC = "$e"

Clear-Host

# ============================================================================
# MERCWAR AI BANNER (SAFE ESCAPED CHARACTER MAP)
# ============================================================================
# Construct character glyphs using Unicode hex values to prevent encoding errors
$b1 = [char]0x2588 # █
$b2 = [char]0x2557 # ╗
$b3 = [char]0x2551 # ║
$b4 = [char]0x2550 # ═
$b5 = [char]0x2554 # ╔
$b6 = [char]0x255A # ╚
$b7 = [char]0x255D # ╝

Write-Host "$ESC[38;2;255;69;0m  =======================================================================$ESC[0m"
Write-Host "$ESC[38;2;255;36;0m  __  __ _____ ____   ____ __        __    _     ____       _    ___  $ESC[0m"
Write-Host "$ESC[38;2;255;94;0m |  \/  | ____|  _ \ / ___|\ \      / /   / \   |  _ \     / \   |_ _| $ESC[0m"
Write-Host "$ESC[38;2;255;140;0m | |\/| |  _| | |_) | |     \ \ /\ / /   / _ \  | |_) |   / _ \   | |  $ESC[0m"
Write-Host "$ESC[38;2;255;180;0m | |  | | |___|  _ <| |___   \ V  V /   / ___ \ |  _ <   / ___ \  | |  $ESC[0m"
Write-Host "$ESC[38;2;255;220;0m |_|  |_|_____|_| \_\\____|   \_/\_/   /_/   \_\|_| \_\ /_/   \_\___| $ESC[0m"
Write-Host "$ESC[38;2;255;69;0m  =======================================================================$ESC[0m"
Write-Host "$ESC[38;2;0;242;255m               [ A R T I F I C I A L   I N T E L L I G E N C E ]       $ESC[0m"
Write-Host "$ESC[38;2;255;69;0m  =====================================================================$ESC[0m"
Write-Host ""
Write-Host "$ESC[96m  Dark-Com-2 Engine Public PowerShell Installer                       $ESC[0m"
Write-Host ""
Write-Host "$ESC[38;2;255;69;0m  ---------------------------------------------------------------------$ESC[0m"
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
} catch {}

$TargetModuleRoot = Join-Path $HOME "Documents\DarkComEngine"

if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "$ESC[33m[INFO] Scanning destination...$ESC[0m"
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

Set-Location $TargetModuleRoot
Write-Host "$ESC[90m[INFO] Downlink active: $(Get-Location)$ESC[0m"
Write-Host ""

$FilesToDownload = @(
    @{
        Name     = "darkcom2.exe"
        Url      = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/refs/heads/main/darkcom2.exe"
        Required = $true
    },
    @{
        Name     = "DarkComHeadless.dll"
        Url      = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/refs/heads/main/DarkComHeadless.dll"
        Required = $true
    },
    @{
        Name     = "WebView2Loader.dll"
        Url      = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/refs/heads/main/WebView2Loader.dll"
        Required = $true
    },
    @{
        Name     = "darkcom2.ps1"
        Url      = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/refs/heads/main/darkcom2.ps1"
        Required = $true
    },
    @{
        Name     = "darkcom2.bat"
        Url      = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/refs/heads/main/darkcom2.bat"
        Required = $true
    }
)

Write-Host "$ESC[94m[INFO] Installing...$ESC[0m"
Write-Host ""

# Progress bar helper using safe character codes
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
    
    # Safe character maps: Solid block (0x2588) and light shade (0x2591)
    $FilledBlock = [char]0x2588
    $UnfilledBlock = [char]0x2591
    
    $Bar = ("$FilledBlock" * $FilledWidth) + ("$UnfilledBlock" * $UnfilledWidth)
    
    $PrintStr = "$ESC[G$ESC[K$ESC[90m[$ESC[96m$Bar$ESC[90m] $ESC[97m$Percent% $ESC[90m| Deploying: $ESC[38;5;111m$FileName$ESC[0m"
    [Console]::Write($PrintStr)
}

$FailedFiles = @()
$TotalFiles = $FilesToDownload.Count
$CurrentIndex = 0

Update-ProgressBar -Current 0 -Total $TotalFiles -FileName "Initializing ..."

foreach ($File in $FilesToDownload) {
    $Destination = Join-Path $TargetModuleRoot $File.Name
    $CurrentIndex++

    try {
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

Update-ProgressBar -Current $TotalFiles -Total $TotalFiles -FileName "Deployment Layout Ready"
Write-Host "`n"

if ($FailedFiles.Count -gt 0) {
    Write-Error "$ESC[91m[FATAL] Missing required file(s): $($FailedFiles -join ', ')$ESC[0m"
    return
}

$ScriptPath = ".\darkcom2.ps1"
$BatPath = ".\darkcom2.bat"

Write-Host "[SUCCESS] Dark-Com-2 installation completed successfully.`n" -ForegroundColor Green

# ============================================================================
# INTERACTIVE KEYBOARD-DRIVEN MENU SELECTION
# ============================================================================

$MenuOptions = @(
    "Run Eclipse",
    "Run DarkCom",
    "Exit"
)
$Selection = 0
$Running = $true

[Console]::CursorVisible = $false

Write-Host "$ESC[33mEclipse will connect to the AVIS-DATALAKE for you$ESC[0m"
Write-Host "$ESC[33m-------------------------------------------------$ESC[0m"
Write-Host ""
Write-Host "$ESC[90mUse UP/DOWN and ENTER to load an interface:$ESC[0m`n"

$StartTop = [Console]::CursorTop

while ($Running) {
    [Console]::SetCursorPosition(0, $StartTop)
    
    for ($i = 0; $i -lt $MenuOptions.Count; $i++) {
        if ($i -eq $Selection) {
            [Console]::Write("$ESC[30;103m  > [X] $($MenuOptions[$i])  $ESC[0m$ESC[K`n")
        } else {
            [Console]::Write("$ESC[97m    [ ] $($MenuOptions[$i]) $ESC[0m$ESC[K`n")
        }
    }
    
    $KeyInfo = [Console]::ReadKey($true)
    
    if ($KeyInfo.Key -eq [ConsoleKey]::UpArrow) {
        if ($Selection -eq 0) { $Selection = $MenuOptions.Count - 1 } else { $Selection-- }
    }
    elseif ($KeyInfo.Key -eq [ConsoleKey]::DownArrow) {
        if ($Selection -eq $MenuOptions.Count - 1) { $Selection = 0 } else { $Selection++ }
    }
    elseif ($KeyInfo.Key -eq [ConsoleKey]::Enter) {
        $Running = $false
    }
}

[Console]::CursorVisible = $true
Clear-Host

if ($Selection -eq 0) {
    Write-Host "$ESC[93m[INFO] Loading Eclipse...$ESC[0m"
    if (Test-Path $BatPath) {
        cls
        & cmd.exe /c "$BatPath"
    } else {
        Write-Error "$ESC[91m[ERROR] Eclipse is missing or inaccessible.$ESC[0m"
    }
} 
elseif ($Selection -eq 1) {
    Write-Host "$ESC[90m[INFO] Loading DarkCom2...$ESC[0m"
    if (Test-Path $ScriptPath) {
        $ProcessArgs = "-NoProfile -ExecutionPolicy Bypass -File `"$ScriptPath`""
        cls
        Start-Process -FilePath "powershell.exe" -ArgumentList $ProcessArgs -NoNewWindow -Wait
    } else {
        Write-Error "$ESC[91m[ERROR] Native PowerShell backend execution script is missing.$ESC[0m"
    }
} 
else {
    Write-Host "$ESC[90m[INFO] Installer exiting. Launch manually: $TargetModuleRoot$ESC[0m"
}
