[CmdletBinding()]
param (
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Url = "https://httpbin.org",

    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Data = $null
)

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " Dark-Com-2 Engine Headless Execution Broker (Fixed)" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($Url)) {
    $Url = "https://httpbin.org"
}

Write-Host "[INFO] Invoking local headless binary..." -ForegroundColor Gray
Write-Host "[INFO] Target URL: $Url" -ForegroundColor White

$TargetDirectory = Join-Path $HOME "Documents\DarkComEngine"
$DllPath = Join-Path $TargetDirectory "DarkComHeadless.dll"

if (-not (Test-Path $DllPath)) {
    Write-Error "[FATAL] Missing core component: $DllPath"
    return
}

# 1. Initialize native structural type bindings cleanly via top-level C# structs
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

# 2. Spin execution off inside a separate engine thread job space to allow the message pump to run
try {
    Write-Host "[INFO] Spinning up background processing thread transaction..." -ForegroundColor Yellow

    # Define a clean worker script block for background job isolation
    $WorkerScript = {
        param($DllDir, $TargetUrl, $PostData)
        
        try {
            $KernelCode = @"
using System;
using System.Runtime.InteropServices;
public class JobKernel32 {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern bool SetDllDirectory(string lpPathName);
}
"@
            Add-Type -TypeDefinition $KernelCode -ErrorAction SilentlyContinue
            [JobKernel32]::SetDllDirectory($DllDir) | Out-Null

            $BridgeCode = @"
using System;
using System.Runtime.InteropServices;
public class JobEngine {
    [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
    public static extern IntPtr RunHeadlessRequest(string url, string postData);
}
"@
            Add-Type -TypeDefinition $BridgeCode -ErrorAction SilentlyContinue

            $Ptr = [JobEngine]::RunHeadlessRequest($TargetUrl, $PostData)
            if ($Ptr -ne [IntPtr]::Zero) {
                return [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($Ptr)
            }
        }
        catch {
            return "JOB_ERROR: $_"
        }
        return $null
    }

    # Start the worker thread execution job
    $BypassJob = Start-Job -ScriptBlock $WorkerScript -ArgumentList $TargetDirectory, $Url, $Data

    # 3. Active UI main console thread message loop pump structure using top-level type definition
    $Msg = New-Object DarkWin32Msg
    while ($BypassJob.State -eq "Running") {
        if ([DarkComEngine]::PeekMessage([ref]$Msg, [IntPtr]::Zero, 0, 0, 1)) {
            [DarkComEngine]::TranslateMessage([ref]$Msg) | Out-Null
            [DarkComEngine]::DispatchMessage([ref]$Msg) | Out-Null
        }
        [System.Threading.Thread]::Sleep(15)
    }

    # Harvest output string from thread workspace boundaries
    $ResponsePayload = Receive-Job -Job $BypassJob -ErrorAction Stop

    if ($ResponsePayload -and $ResponsePayload.ToString().StartsWith("JOB_ERROR:")) {
        Write-Error "[ERROR] Background engine failure: $($ResponsePayload)"
    }
    elseif ($ResponsePayload) {
        Write-Host "`n[RESPONSE DATA]:" -ForegroundColor Green
        return $ResponsePayload
    } else {
        Write-Error "[ERROR] Headless browser execution failed or returned an invalid null string object parameter reference."
    }
}
catch {
    Write-Error "[FATAL] An error crashed the processing loop: $_"
}
finally {
    if ($BypassJob) {
        Remove-Job -Job $BypassJob -Force
    }
}
