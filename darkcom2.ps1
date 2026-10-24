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

# 1. Initialize native structural type bindings & kernel DLL directory setting
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
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern bool SetDllDirectory(string lpPathName);

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

# 2. Set DLL search directory context to ensure dependencies resolve cleanly
try {
    [DarkComEngine]::SetDllDirectory($TargetDirectory) | Out-Null
} catch {}

# 3. Execute request and run message pump synchronously/safely
try {
    Write-Host "[INFO] Executing headless transaction..." -ForegroundColor Yellow

    # Run the headless request
    $Ptr = [DarkComEngine]::RunHeadlessRequest($Url, $Data)

    $ResponsePayload = $null
    if ($Ptr -ne [IntPtr]::Zero) {
        $ResponsePayload = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($Ptr)
    }

    if ($ResponsePayload) {
        Write-Host "`n[GATEWAY RESPONSE DATA]:" -ForegroundColor Green
        return $ResponsePayload
    } else {
        Write-Error "[ERROR] Headless browser execution returned a null pointer or empty string response."
    }
}
catch {
    Write-Error "[FATAL] An error crashed the execution loop: $_"
}
