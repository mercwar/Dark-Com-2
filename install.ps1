[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Global PowerShell Installer & Menu" -ForegroundColor Cyan
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
# (Matches structure-bound command line arguments: url, method, postData, isVerbose)
$NativeBridgeCode = @"
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
public struct CommandLineArgs {
    public IntPtr url;
    public IntPtr method;
    public IntPtr postData;
    public int isVerbose;
}

public class DarkComEngine {
    [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr RunHeadlessRequest(ref CommandLineArgs args);
}
"@

if (-not ([System.Management.Automation.PSTypeName]'DarkComEngine').Type) {
    Add-Type -TypeDefinition $NativeBridgeCode -ErrorAction SilentlyContinue
}

# 5. Define Invoke-DarkCurl function in global scope
function global:Invoke-DarkCurl {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$Url,

        [Parameter(Mandatory = $false, Position = 1)]
        [string]$Method = "GET",

        [Parameter(Mandatory = $false, Position = 2)]
        [string]$Data = $null,

        [Parameter(Mandatory = $false)]
        [switch]$VerboseLog
    )

    process {
        try {
            $argsStruct = New-Object CommandLineArgs
            $argsStruct.url = [System.Runtime.InteropServices.Marshal]::StringToHSTRUnmanaged($Url)
            
            if ($Data -and -not $PSBoundParameters.ContainsKey('Method')) {
                $Method = "POST"
            }

            $argsStruct.method = [System.Runtime.InteropServices.Marshal]::StringToHSTRUnmanaged($Method)
            if ($Data) {
                $argsStruct.postData = [System.Runtime.InteropServices.Marshal]::StringToHSTRUnmanaged($Data)
            } else {
                $argsStruct.postData = [IntPtr]::Zero
            }
            $argsStruct.isVerbose = if ($VerboseLog) { 1 } else { 0 }

            $ResultBufferPtr = [DarkComEngine]::RunHeadlessRequest([ref]$argsStruct)

            if ($ResultBufferPtr -ne [IntPtr]::Zero) {
                [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($ResultBufferPtr)
            } else {
                Write-Error "[ERROR] Execution returned a null buffer pointer."
            }

            # Cleanup unmanaged allocations
            [System.Runtime.InteropServices.Marshal]::FreeHSTRUnmanaged($argsStruct.url)
            if ($argsStruct.method -ne [IntPtr]::Zero) { [System.Runtime.InteropServices.Marshal]::FreeHSTRUnmanaged($argsStruct.method) }
            if ($argsStruct.postData -ne [IntPtr]::Zero) { [System.Runtime.InteropServices.Marshal]::FreeHSTRUnmanaged($argsStruct.postData) }
        }
        catch {
            Write-Error "[FATAL] Interop transaction failed: $_"
        }
    }
}

# 6. Define the Interactive State Broker Menu (dmenu) in global scope
function global:Invoke-DarkMenu {
    [CmdletBinding()]
    param()

    $Endpoint = "https://cron.iblogger.org/darkcom2/index.php"

    while ($true) {
        Clear-Host
        Write-Host "============================================================" -ForegroundColor Cyan
        Write-Host "        DARK-COM-2 STATE BROKER CONTROL PANEL (PowerShell)" -ForegroundColor Cyan
        Write-Host "============================================================" -ForegroundColor Cyan
        Write-Host " [1] Read Application (Public)"
        Write-Host " [2] Write Application (Public)"
        Write-Host " [3] Read Application (Private)"
        Write-Host " [4] Write Application (Private)"
        Write-Host " [5] Read Session (Public)"
        Write-Host " [6] Write Session (Public)"
        Write-Host " [7] Read Session (Private)"
        Write-Host " [8] Write Session (Private)"
        Write-Host " [9] Exit"
        Write-Host "============================================================" -ForegroundColor Cyan
        
        $choice = Read-Host "Select option [1-9]"

        switch ($choice) {
            "1" {
                Write-Host "`n[*] Fetching Application Public State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=true&application=true&write=false" -Method GET
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "2" {
                Write-Host "`n"
                $jsonData = Read-Host 'Enter JSON payload (e.g. {"status":"online"})'
                Write-Host "[*] Writing to Application Public State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=true&application=true&write=true" -Method POST -Data $jsonData
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "3" {
                Write-Host "`n[*] Fetching Application Private State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=false&application=true&write=false" -Method GET
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "4" {
                Write-Host "`n"
                $jsonData = Read-Host 'Enter JSON payload'
                Write-Host "[*] Writing to Application Private State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=false&application=true&write=true" -Method POST -Data $jsonData
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "5" {
                Write-Host "`n[*] Fetching Session Public State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=true&application=false&write=false" -Method GET
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "6" {
                Write-Host "`n"
                $jsonData = Read-Host 'Enter JSON payload'
                Write-Host "[*] Writing to Session Public State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=true&application=false&write=true" -Method POST -Data $jsonData
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "7" {
                Write-Host "`n[*] Fetching Session Private State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=false&application=false&write=false" -Method GET
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "8" {
                Write-Host "`n"
                $jsonData = Read-Host 'Enter JSON payload'
                Write-Host "[*] Writing to Session Private State..." -ForegroundColor Yellow
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Invoke-DarkCurl "$Endpoint?public=false&application=false&write=true" -Method POST -Data $jsonData
                Write-Host "------------------------------------------------------------" -ForegroundColor Gray
                Read-Host "Press Enter to continue"
            }
            "9" {
                return
            }
            default {
                Write-Host "[!] Invalid option. Please select 1-9." -ForegroundColor Red
                Start-Sleep -Seconds 1
            }
        }
    }
}

# Set global aliases
Set-Alias -Name dcurl -Value Invoke-DarkCurl -Scope Global -Force
Set-Alias -Name dmenu -Value Invoke-DarkMenu -Scope Global -Force

Write-Host ""
Write-Host "[SUCCESS] Dark-Com-2 functions and aliases loaded into memory!" -ForegroundColor Green
Write-Host "Commands available:" -ForegroundColor White
Write-Host "  dcurl <URL> [-Method GET/POST] [-Data <JSON>]" -ForegroundColor Yellow
Write-Host "  dmenu (Launches the Interactive State Broker Control Panel)" -ForegroundColor Yellow
Write-Host ""
