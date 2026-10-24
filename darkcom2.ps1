[CmdletBinding()]
param (
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Url = "https://httpbin.org",

    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Data = $null
)

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Headless Execution Broker (Native)" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($Url)) {
    $Url = "https://httpbin.org"
}

$TargetDirectory = Join-Path $HOME "Documents\DarkComEngine"
$DllPath = Join-Path $TargetDirectory "DarkComHeadless.dll"

if (-not (Test-Path $DllPath)) {
    Write-Error "[FATAL] Missing core component: $DllPath"
    return
}

# 1. Set DLL directory explicitly for the current process
try {
    $KernelCode = @"
using System;
using System.Runtime.InteropServices;
public class EngineKernel32 {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern bool SetDllDirectory(string lpPathName);
}
"@
    Add-Type -TypeDefinition $KernelCode -ErrorAction SilentlyContinue
    [EngineKernel32]::SetDllDirectory($TargetDirectory) | Out-Null
}
catch {
    Write-Error "[FATAL] Failed to configure DLL search path: $_"
    return
}

# 2. Initialize native structural bindings and Direct Interop
try {
    $NativeBridgeCode = @"
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public struct DarkWin32Msg {
    public IntPtr hwnd;
    public uint message;
    public IntPtr wParam;
    public IntPtr lParam;
    public uint time;
    public int pt_x;
    public int pt_y;
}

public class DarkComEngine {
    [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
    public static extern IntPtr RunHeadlessRequest(string url, string postData);

    [DllImport("user32.dll")]
    public static extern bool PeekMessage(out DarkWin32Msg lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax, uint wRemoveMsg);

    [DllImport("user32.dll")]
    public static extern bool TranslateMessage(ref DarkWin32Msg lpMsg);

    [DllImport("user32.dll")]
    public static extern IntPtr DispatchMessage(ref DarkWin32Msg lpMsg);
}
"@
    if (-not ([System.Management.Automation.PSTypeName]'DarkComEngine').Type) {
        Add-Type -TypeDefinition $NativeBridgeCode -ErrorAction SilentlyContinue
    }
}
catch {
    Write-Error "[FATAL] Failed to configure Interop type layouts: $_"
    return
}

# 3. Execute Native Request directly with a message loop pump
try {
    Write-Host "[INFO] Invoking headless request for: $Url" -ForegroundColor Yellow

    # Call native DLL entry point directly
    $Ptr = [DarkComEngine]::RunHeadlessRequest($Url, $Data)

    if ($Ptr -ne [IntPtr]::Zero) {
        $ResponsePayload = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($Ptr)
        Write-Host "`n[RESPONSE DATA]:" -ForegroundColor Green
        return $ResponsePayload
    } else {
        Write-Error "[ERROR] Headless execution returned a null pointer reference."
    }
}
catch {
    Write-Error "[FATAL] An error occurred during native execution: $_"
}
