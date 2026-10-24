[CmdletBinding()]
param()

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Target your explicit static raw storage distribution path
$BaseUri = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/main/"

# 2. Establish target module installation folder
$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"
if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "[INFO] Creating destination module infrastructure layout..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

# 3. Enumerate target source components
$FilesToDownload = @(
    "DarkComHeadless.dll",
    "WebView2Loader.dll",
    "DarkCurl.psm1"
)

# 4. Stream application binaries directly from the raw GitHub context layout
Write-Host "[INFO] Fetching core engine deployment layers from GitHub..." -ForegroundColor Yellow
foreach ($FileName in $FilesToDownload) {
    # Append a randomized cache buster query string to bypass stale edge server caching
    $SourceUrl   = "$BaseUri$FileName?cb=$(Get-Random)"
    $Destination = Join-Path $TargetModuleRoot $FileName

    try {
        Write-Host " -> Downloading: $FileName" -ForegroundColor Gray
        
        # Invoke-WebRequest forces raw stream processing to ensure clean DLL alignment on disk
        Invoke-WebRequest -Uri $SourceUrl -OutFile $Destination -TimeoutSec 30 -ErrorAction Stop
    }
    catch {
        Write-Warning "[WARN] Failed to pull $FileName from raw endpoint. Checking local backup context..."
        if (-not (Test-Path $Destination)) {
            Write-Error "[FATAL] Critical file missing and remote acquisition failed: $FileName"
            return
        }
    }
}

# 5. Native session integration verification
Write-Host "[INFO] Registering module variables into current terminal profile parameters..." -ForegroundColor Yellow
try {
    # Point directly to the newly dropped psm1 container layout context
    Import-Module -Name (Join-Path $TargetModuleRoot "DarkCurl.psm1") -Force -ErrorAction Stop
    
    Write-Host "`n[SUCCESS] DarkCurl Engine Pipeline Registered Natively into your PowerShell environment!" -ForegroundColor Green
    Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
    Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
    Write-Host "Usage POST:    dcurl 'https://httpbin.org' 'foo=bar&baz=qux'`n" -ForegroundColor White
}
catch {
    Write-Error "[FATAL] Failed to initialize module path context layout natively: $_"
}
