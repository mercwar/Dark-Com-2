[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
# Force PowerShell to hide the heavy default blue progress rendering overlay
$ProgressPreference = "SilentlyContinue"

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

Write-Host "$ESC[94m[INFO] Fetching core engine deployment layers from GitHub...$ESC[0m"
Write-Host ""

# Helper function to paint a modern flat block progress bar safely inline
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
    
    # [FIX] Explicitly force the cursor to Column 0 and clear everything to the right ($ESC[K) 
    # to wipe out old text from previous runs or overlapping filenames.
    $PrintStr = "$ESC[G$ESC[K$ESC[90m[$ESC[96m$Bar$ESC[90m] $ESC[97m$Percent% $ESC[90m| Deploying: $ESC[38;5;111m$FileName$ESC[0m"
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

Write-Host "[SUCCESS] Dark-Com-2 Engine infrastructure deployed successfully.`n" -ForegroundColor Green

# ============================================================================
# INTERACTIVE KEYBOARD-DRIVEN MENU SELECTION (FIXED RENDERING)
# ============================================================================

$MenuOptions = @(
    "Run darkcom2.bat (State Broker Interactive Menu)",
    "Run darkcom2.ps1 (Headless Runspace Broker Thread)",
    "Exit Installer Sequence"
)
$Selection = 0
$Running = $true

# Hide terminal cursor while browsing the selection list
[Console]::CursorVisible = $false

Write-Host "$ESC[33mWould you like to run Eclipse now?$ESC[0m"
Write-Host "$ESC[90mUse UP/DOWN arrow keys and press ENTER to confirm choice:$ESC[0m`n"

# Capture the baseline starting line for absolute cursor placement
$StartTop = [Console]::CursorTop

while ($Running) {
    # Jump back to the exact menu rendering anchor point
    [Console]::SetCursorPosition(0, $StartTop)
    
    for ($i = 0; $i -lt $MenuOptions.Count; $i++) {
        if ($i -eq $Selection) {
            Write-Host "$ESC[30;103m  > [X] $($MenuOptions[$i])  $ESC[0m"
        } else {
            Write-Host "$ESC[97m    [ ] $($MenuOptions[$i]) $ESC[0m"
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

# Restore default console visibility properties
[Console]::CursorVisible = $true
Clear-Host

# Process branch targets based on final menu coordinate selection context
if ($Selection -eq 0) {
    Write-Host "$ESC[93m[INFO] Invoking state broker interactive console menu...$ESC[0m"
    if (Test-Path $BatPath) {
		cls
        & cmd.exe /c "$BatPath"
    } else {
        Write-Error "$ESC[91m[ERROR] Batch routing asset is missing or inaccessible.$ESC[0m"
    }
} 
elseif ($Selection -eq 1) {
    Write-Host "$ESC[90m[INFO] Bypassing execution policies for headless Runspace Broker thread...$ESC[0m"
    if (Test-Path $ScriptPath) {
        $ProcessArgs = "-NoProfile -ExecutionPolicy Bypass -File `"$ScriptPath`""
		cls
        Start-Process -FilePath "powershell.exe" -ArgumentList $ProcessArgs -NoNewWindow -Wait
    } else {
        Write-Error "$ESC[91m[ERROR] Native PowerShell backend execution script is missing.$ESC[0m"
    }
} 
else {
    Write-Host "$ESC[90m[INFO] Installer sequence exiting. Launch your chosen protocol manually inside $TargetModuleRoot$ESC[0m"
}
