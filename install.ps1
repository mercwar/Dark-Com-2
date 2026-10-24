[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Global PowerShell Installer" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

# 1. Establish directory layout for native binaries
$TargetDirectory = Join-Path $HOME "Documents\DarkComEngine"

if (-not (Test-Path $TargetDirectory)) {
    Write-Host "[INFO] Creating binary storage directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $TargetDirectory -Force | Out-Null
}

# 2. Download native DLL binaries
$FilesToDownload = @(
    @{
        Name = "DarkComHeadless.dll"
        Url  = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/DarkComHeadless.dll"
    },
    @{
        Name = "WebView2Loader.dll"
        Url  = "https://github.com/mercwar/Dark-Com-2/raw/refs/heads/main/WebView2Loader.dll"
    }
)

Write-Host "[INFO] Fetching core engine binaries..." -ForegroundColor Yellow

foreach ($File in $FilesToDownload) {
    $Destination = Join-Path $TargetDirectory $File.Name

    try {
        Write-Host " -> Downloading: $($File.Name)" -ForegroundColor Gray

        Invoke-WebRequest `
            -Uri $File.Url `
            -OutFile $Destination `
            -UseBasicParsing `
            -TimeoutSec 30 `
            -ErrorAction Stop

        if (-not (Test-Path $Destination) -or (Get-Item $Destination).Length -eq 0) {
            throw "Downloaded binary is empty or missing."
        }

        Write-Host "    [OK] $($File.Name)" -ForegroundColor Green
    }
    catch {
        Write-Error "[FATAL] Failed to download $($File.Name): $_"
        return
    }
}

# 3. Configure DLL Search Path in-memory
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class DarkKernel32 {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern bool SetDllDirectory(string lpPathName);
}
"@ -ErrorAction SilentlyContinue

[DarkKernel32]::SetDllDirectory($TargetDirectory) | Out-Null

# 4. Compile unmanaged C# interop bridge directly in-memory
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

# 5. Define function directly in memory session
function global:Invoke-DarkCurl {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$Url,

        [Parameter(Mandatory = $false, Position = 1)]
        [string]$Data = $null
    )

    process {
        try {
            $ResultBufferPtr = [DarkComEngine]::RunHeadlessRequest($Url, $Data)

            if ($ResultBufferPtr -ne [IntPtr]::Zero) {
                return [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($ResultBufferPtr)
            } else {
                Write-Error "[ERROR] Execution returned a null buffer pointer."
            }
        }
        catch {
            Write-Error "[FATAL] Interop transaction failed: $_"
        }
    }
}

# Set alias in global scope
Set-Alias -Name dcurl -Value Invoke-DarkCurl -Scope Global -Force

Write-Host ""
Write-Host "[SUCCESS] DarkCurl function and 'dcurl' alias successfully loaded into memory!" -ForegroundColor Green
Write-Host "Usage Command: dcurl <URL>" -ForegroundColor White
Write-Host "Usage Example: dcurl 'https://httpbin.org'" -ForegroundColor White
Write-Host "Usage POST:    dcurl 'https://httpbin.org/post' 'foo=bar&baz=qux'" -ForegroundColor White
Write-Host ""
