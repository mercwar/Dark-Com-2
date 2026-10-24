[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Module Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
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
    }
)

Write-Host "[INFO] Fetching core engine binaries from GitHub..." -ForegroundColor Yellow

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

# Generate DarkCurl.psm1 locally to avoid remote module parsing issues
Write-Host "[INFO] Generating local module wrapper script..." -ForegroundColor Yellow
$ModuleScriptContent = @'
# ====================================================================
# AIFVS-MODULE: DarkCurl.psm1
# PowerShell Command-Line Wrapper for DarkComHeadless WebView2 Engine
# ====================================================================

$NativeDllPath = $PSScriptRoot
if (Test-Path $NativeDllPath) {
    Add-Type -TypeDefinition @"
    using System;
    using System.Runtime.InteropServices;
    public class DarkKernel32 {
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern bool SetDllDirectory(string lpPathName);
    }
"@ -ErrorAction SilentlyContinue
    [DarkKernel32]::SetDllDirectory($NativeDllPath) | Out-Null
}

$NativeBridgeCode = @"
using System;
using System.Runtime.InteropServices;

public class DarkComEngine {
    [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
    public static extern IntPtr RunHeadlessRequest(string url, string postData);
}
"@

if (-not ([System.Management.Automation.PSTypeName]'DarkComEngine').Type) {
    Add-Type -TypeDefinition $NativeBridgeCode -ErrorAction SilentlyContinue
}

function Invoke-DarkCurl {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true, Position = 0, HelpMessage = "Target URL endpoint.")]
        [string]$Url,

        [Parameter(Mandatory = $false, Position = 1, HelpMessage = "Optional raw data payload for POST.")]
        [string]$Data = $null
    )

    process {
        try {
            $ResultBufferPtr = [DarkComEngine]::RunHeadlessRequest($Url, $Data)

            if ($ResultBufferPtr -ne [IntPtr]::Zero) {
                $ExtractedStringOutput = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($ResultBufferPtr)
                return $ExtractedStringOutput
            } else {
                Write-Error "[ERROR] Headless execution returned a null buffer pointer."
            }
        }
        catch {
            Write-Error "[FATAL] Interop transaction failed: $_"
        }
    }
}

Export-ModuleMember -Function Invoke-DarkCurl
Set-Alias -Name dcurl -Value Invoke-DarkCurl -Force
'@

$ModulePath = Join-Path $TargetModuleRoot "DarkCurl.psm1"
Set-Content -Path $ModulePath -Value $ModuleScriptContent -Encoding UTF8

Write-Host "[INFO] Registering module into current session..." -ForegroundColor Yellow

try {
    Import-Module $ModulePath -Force -ErrorAction Stop

    Write-Host ""
    Write-Host "[SUCCESS] DarkCurl Engine Pipeline Successfully Installed & Registered!" -ForegroundColor Green
    Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
    Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
    Write-Host "Usage POST:    dcurl 'https://httpbin.org/post' 'foo=bar&baz=qux'" -ForegroundColor White
    Write-Host ""
}
catch {
    Write-Error "[FATAL] Failed to initialize module: $($_.Exception.Message)"
}
