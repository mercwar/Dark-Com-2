[CmdletBinding()]
param (
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Url = "https://httpbin.org",

    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Data = $null,

    [Parameter(Mandatory = $false)]
    [switch]$VerboseLog
)

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Headless Execution Broker (Fixed) " -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($Url)) {
    $Url = "https://httpbin.org"
}

Write-Host "[INFO] Initializing native headless engine..." -ForegroundColor Gray
Write-Host "[INFO] Target URL: $Url" -ForegroundColor White

$TargetDirectory = Join-Path $HOME "Documents\DarkComEngine"
$DllPath = Join-Path $TargetDirectory "DarkComHeadless.dll"

if (-not (Test-Path $DllPath)) {
    Write-Error "[FATAL] Missing core component: $DllPath"
    return
}

# Set DLL search directory so Windows can resolve WebView2 loader dependencies correctly
$KernelCode = @"
using System;
using System.Runtime.InteropServices;
public class Kernel32Bridge {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern bool SetDllDirectory(string lpPathName);
}
"@
Add-Type -TypeDefinition $KernelCode -ErrorAction SilentlyContinue
[Kernel32Bridge]::SetDllDirectory($TargetDirectory) | Out-Null

# Define the exact CommandLineArgs struct matching your C header layout
$InteropCode = @"
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
public struct CommandLineArgs {
    public IntPtr url;
    public IntPtr method;
    public IntPtr postData;
    public int isVerbose;
}

public class DarkComInterop {
    [DllImport("DarkComHeadless.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr RunHeadlessRequest(ref CommandLineArgs args);
}
"@

if (-not ([System.Management.Automation.PSTypeName]'DarkComInterop').Type) {
    Add-Type -TypeDefinition $InteropCode -ErrorAction SilentlyContinue
}

try {
    Write-Host "[Uplinking] Dcom..." -ForegroundColor Yellow

    # Allocate unmanaged memory strings for the C struct pointers
    $pUrl = [System.Runtime.InteropServices.Marshal]::StringToHGlobalUni($Url)
    
    $methodStr = if ([string]::IsNullOrEmpty($Data)) { "GET" } else { "POST" }
    $pMethod = [System.Runtime.InteropServices.Marshal]::StringToHGlobalUni($methodStr)
    
    $pPostData = if (-not [string]::IsNullOrEmpty($Data)) { 
        [System.Runtime.InteropServices.Marshal]::StringToHGlobalUni($Data) 
    } else { 
        [IntPtr]::Zero 
    }

    $config = New-Object CommandLineArgs
    $config.url = $pUrl
    $config.method = $pMethod
    $config.postData = $pPostData
    $config.isVerbose = if ($VerboseLog) { 1 } else { 0 }

    Write-Host "[Downlinking] Spawning Dcom..." -ForegroundColor Gray

    # Execute the request directly in the current synchronization context
    $ResultPtr = [DarkComInterop]::RunHeadlessRequest([ref]$config)

    if ($ResultPtr -ne [IntPtr]::Zero) {
        $ResponsePayload = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($ResultPtr)
        Write-Host "`n[RESPONSE DATA]:" -ForegroundColor Green
        [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
        $ResponsePayload
    } else {
        Write-Error "[ERROR] Network transaction returned empty or null data tracking frames."
    }
}
catch {
    Write-Error "[FATAL] Execution failed: $_"
}
finally {
    # Clean up unmanaged heap allocations
    if ($pUrl -and $pUrl -ne [IntPtr]::Zero) { [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pUrl) }
    if ($pMethod -and $pMethod -ne [IntPtr]::Zero) { [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pMethod) }
    if ($pPostData -and $pPostData -ne [IntPtr]::Zero) { [System.Runtime.InteropServices.Marshal]::FreeHGlobal($pPostData) }
}
