[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# Force modern TLS
try {
    [Net.ServicePointManager]::SecurityProtocol = `
        [Net.SecurityProtocolType]::Tls12
}
catch {}

$BaseUri = "https://raw.githubusercontent.com/mercwar/Dark-Com-2/main/"

$TargetModuleRoot = Join-Path $HOME "Documents\WindowsPowerShell\Modules\DarkCurl"

if (-not (Test-Path $TargetModuleRoot))
{
    Write-Host "[INFO] Creating module directory..." -ForegroundColor Yellow

    New-Item `
        -ItemType Directory `
        -Path $TargetModuleRoot `
        -Force | Out-Null
}

# Mark files as required/optional
$FilesToDownload = @(
    @{
        Name = "DarkComHeadless.dll"
        Required = $false
    },
    @{
        Name = "WebView2Loader.dll"
        Required = $true
    },
    @{
        Name = "DarkCurl.psm1"
        Required = $true
    }
)

function Test-RemoteFile
{
    param(
        [string]$Url
    )

    try
    {
        $Response = Invoke-WebRequest `
            -Uri $Url `
            -Method Head `
            -UseBasicParsing `
            -TimeoutSec 15

        return ($Response.StatusCode -ge 200 -and $Response.StatusCode -lt 300)
    }
    catch
    {
        return $false
    }
}

function Get-SafeFile
{
    param(
        [string]$Url,
        [string]$Destination
    )

    try
    {
        Invoke-WebRequest `
            -Uri $Url `
            -OutFile $Destination `
            -UseBasicParsing `
            -TimeoutSec 30 `
            -ErrorAction Stop

        if (-not (Test-Path $Destination))
        {
            throw "File was not created."
        }

        $Size = (Get-Item $Destination).Length

        if ($Size -eq 0)
        {
            throw "Downloaded file is empty."
        }

        return $true
    }
    catch
    {
        Write-Warning $_.Exception.Message
        return $false
    }
}

Write-Host ""
Write-Host "[INFO] Fetching deployment files..." -ForegroundColor Yellow

$Failures = @()

foreach ($Entry in $FilesToDownload)
{
    $FileName = $Entry.Name
    $Required = $Entry.Required

    $Url = "$BaseUri$FileName"

    $Destination = Join-Path $TargetModuleRoot $FileName

    Write-Host " -> Processing: $FileName" -ForegroundColor Gray

    if (-not (Test-RemoteFile $Url))
    {
        if ($Required)
        {
            Write-Host "    [ERROR] Remote file missing." -ForegroundColor Red
            $Failures += $FileName
        }
        else
        {
            Write-Warning "Optional component not found: $FileName"
        }

        continue
    }

    if (Get-SafeFile -Url $Url -Destination $Destination)
    {
        $SizeKB = [math]::Round(
            ((Get-Item $Destination).Length / 1KB),
            2
        )

        Write-Host "    [OK] Downloaded ($SizeKB KB)" -ForegroundColor Green
    }
    else
    {
        if ($Required)
        {
            $Failures += $FileName
        }
    }
}

if ($Failures.Count -gt 0)
{
    Write-Host ""
    Write-Host "[FATAL] Required files missing:" -ForegroundColor Red

    $Failures | ForEach-Object {
        Write-Host " - $_" -ForegroundColor Red
    }

    return
}

$ModulePath = Join-Path $TargetModuleRoot "DarkCurl.psm1"

if (-not (Test-Path $ModulePath))
{
    Write-Error "DarkCurl.psm1 not found after installation."
    return
}

Write-Host ""
Write-Host "[INFO] Loading module..." -ForegroundColor Yellow

try
{
    # Fixed the merged string preference token right here
    Import-Module $ModulePath -Force -ErrorAction Stop
    
    Write-Host ""
    Write-Host "[SUCCESS] DarkCurl installed successfully." -ForegroundColor Green
    Write-Host ""
    Write-Host "Module Location:" -ForegroundColor Cyan
    Write-Host $TargetModuleRoot
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Cyan
    Write-Host "dcurl 'https://httpbin.org'"
    Write-Host "dcurl 'https://httpbin.org' 'foo=bar'"
}
catch
{
    Write-Error "Module import failed:`n$($_.Exception.Message)"
}
