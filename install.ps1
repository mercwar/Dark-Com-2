[CmdletBinding()]
param()

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Define repository target paths and API endpoints
$RepoOwner  = "mercwar"
$RepoName   = "Dark-Com-2"
$Branch     = "main"

# 2. Establish target module installation folder
$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"
if (-not (Test-Path $TargetModuleRoot)) {
    Write-Host "[INFO] Creating destination module infrastructure layout..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetModuleRoot -Force | Out-Null
}

# 3. Enumerate layout target files
$FilesToDownload = @(
    "DarkComHeadless.dll",
    "WebView2Loader.dll",
    "DarkCurl.psm1"
)

# 4. Perform live download loop using the GitHub API stream for binaries
Write-Host "[INFO] Fetching core engine deployment layers from GitHub..." -ForegroundColor Yellow
foreach ($FileName in $FilesToDownload) {
    # Using the GitHub API stream ensures binary data integrity for DLLs
    $SourceUrl   = "https://github.com"
    $Destination = Join-Path $TargetModuleRoot $FileName

    try {
        Write-Host " -> Downloading: $FileName" -ForegroundColor Gray
        
        # Request the metadata from the API to safely pull the download URL
        $FileMetadata = Invoke-RestMethod -Uri $SourceUrl -Headers @{"Accept" = "application/vnd.github.v3+json"} -ErrorAction Stop
        $DownloadUrl  = $FileMetadata.download_url
        
        # Stream the raw application binary down to the target path
        Invoke-WebRequest -Uri $DownloadUrl -OutFile $Destination -ErrorAction Stop
    }
    catch {
        Write-Warning "[WARN] Failed to pull $FileName via API context. Checking local availability..."
        if (-not (Test-Path $Destination)) {
            Write-Error "[FATAL] Critical file missing and remote acquisition failed: $FileName"
            return
        }
    }
}

# 5. Native session integration verification
Write-Host "[INFO] Registering module variables into current terminal profile parameters..." -ForegroundColor Yellow
try {
    Import-Module -Name (Join-Path $TargetModuleRoot "DarkCurl.psm1") -Force -ErrorAction Stop
    
    Write-Host "`n[SUCCESS] DarkCurl Engine Pipeline Registered Natively into your PowerShell environment!" -ForegroundColor Green
    Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
    Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
    Write-Host "Usage POST:    dcurl 'https://httpbin.org' 'foo=bar&baz=qux'`n" -ForegroundColor White
}
catch {
    Write-Error "[FATAL] Failed to initialize module path context layout natively: $_"
}
