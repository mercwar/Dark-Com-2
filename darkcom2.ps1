[CmdletBinding()]
param (
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Url = "https://mercwar01.byethost3.com/darkcom2/index.php",

    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Data = $null
)

$ErrorActionPreference = "Stop"

Write-Host "=====================================================================" -ForegroundColor Cyan
Write-Host " GATEWAY ENGINE :: ECLIPSE PROTOCOL BROKER" -ForegroundColor Cyan
Write-Host "=====================================================================" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($Url)) {
    $Url = "https://mercwar01.byethost3.com/darkcom2/index.php"
}

Write-Host "[INFO] Invoking local headless binary..." -ForegroundColor Gray
Write-Host "[INFO] Target Endpoint: $Url" -ForegroundColor White

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
        Add-Type -TypeDefinition $NativeBridgeCode -ErrorAction Stop
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
        
        # Configure DLL directory layout context inside background thread boundaries
        $KernelCode = @"
        using System;
        using System.Runtime.InteropServices;
        public class JobKernel32 {
            [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
            public static extern bool SetDllDirectory(string lpPathName);
        }
"@
        try { Add-Type -TypeDefinition $KernelCode -ErrorAction SilentlyContinue } catch {}
        [JobKernel32]::SetDllDirectory($DllDir) | Out-Null

        $BridgeCode = @"
        using System;
        using System.Runtime.InteropServices;
        public class JobEngine {
            [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
            public static extern IntPtr RunHeadlessRequest(string url, string postData);
        }
"@
        try { Add-Type -TypeDefinition $BridgeCode -ErrorAction SilentlyContinue } catch {}

        # Run the target headless transaction loop natively
        $Ptr = [JobEngine]::RunHeadlessRequest($TargetUrl, $PostData)
        if ($Ptr -ne [IntPtr]::Zero) {
            return [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($Ptr)
        }
        return $null
    }

    # Start the worker thread execution job
    $BypassJob = Start-Job -ScriptBlock $WorkerScript -ArgumentList $TargetDirectory, $Url, $Data

    # 3. Active UI main console thread message loop pump structure using top-level type definition
    $Msg = New-Object DarkWin32Msg
    while ($BypassJob.State -eq "Running") {
        # Pump Windows messaging context objects to keep the underlying WebView2 alive
        if ([DarkComEngine]::PeekMessage([ref]$Msg, [IntPtr]::Zero, 0, 0, 1)) {
            [DarkComEngine]::TranslateMessage([ref]$Msg) | Out-Null
            [DarkComEngine]::DispatchMessage([ref]$Msg) | Out-Null
        }
        [System.Threading.Thread]::Sleep(15)
    }

    # Harvest output string from thread workspace boundaries
    $ResponsePayload = Receive-Job -Job $BypassJob -ErrorAction Stop

    if ($ResponsePayload) {
        Write-Host "`n[GATEWAY RESPONSE DATA]:" -ForegroundColor Green
        return $ResponsePayload
    } else {
        Write-Error "[ERROR] Headless browser execution failed or returned an invalid null string object parameter reference."
    }
}
catch {
    Write-Error "[FATAL] An error crashed the processing loop: $_"
}
finally {
    # Always scrub and clean background jobs out of thread space memory pools
    if ($BypassJob) {
        Remove-Job -Job $BypassJob -Force
    }
}
