[CmdletBinding()]
param (
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Url = "https://httpbin.org",

    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Data = $null
)

$ErrorActionPreference = "Stop"

# Enable ANSI escape processing in modern Windows terminals
$e = [char]27
$ESC = "$e"

Clear-Host
Write-Host "$ESC[96m=====================================================$ESC[0m"
Write-Host "$ESC[96m Dark-Com-2 Engine Headless Execution Broker (Fixed) $ESC[0m"
Write-Host "$ESC[96m=====================================================$ESC[0m"

if ([string]::IsNullOrWhiteSpace($Url)) {
    $Url = "https://httpbin.org"
}

Write-Host "$ESC[90m[INFO] Invoking local headless binary...$ESC[0m"
Write-Host "$ESC[97m[INFO] Target URL: $Url$ESC[0m"

$TargetDirectory = Join-Path $HOME "Documents\DarkComEngine"
$DllPath = Join-Path $TargetDirectory "DarkComHeadless.dll"

if (-not (Test-Path $DllPath)) {
    Write-Error "$ESC[91m[FATAL] Missing core component: $DllPath$ESC[0m"
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
    Write-Error "$ESC[91m[FATAL] Failed to configure Interop type layouts: $_$ESC[0m"
    return
}

# 2. Establish DLL path search context context to ensure dependencies load cleanly
try {
    [DarkComEngine]::SetDllDirectory($TargetDirectory) | Out-Null
} catch {}

# 3. Spin execution off inside an STA Runspace Thread instead of an isolated background Job process
try {
    Write-Host "$ESC[93m[INFO] Spawning safe STA runspace thread transaction...$ESC[0m"

    $Runspace = [runspacefactory]::CreateRunspace()
    $Runspace.ApartmentState = "STA"
    $Runspace.ThreadOptions = "UseNewThread"
    $Runspace.Open()

    $PowerShellPipeline = [powershell]::Create()
    $PowerShellPipeline.Runspace = $Runspace

    # The processing block mapped to identical runtime boundary pools
    $ScriptBlock = {
        param($TargetUrl, $PostData)
        try {
            $Ptr = [DarkComEngine]::RunHeadlessRequest($TargetUrl, $PostData)
            if ($Ptr -ne [IntPtr]::Zero) {
                return [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($Ptr)
            }
        } catch {
            return "Internal Runspace Thread Exception: " + $_.Exception.Message
        }
        return $null
    }

    $PowerShellPipeline.AddScript($ScriptBlock).AddArgument($Url).AddArgument($Data) | Out-Null
    
    # Asynchronously kick off execution loop frame
    $AsyncResult = $PowerShellPipeline.BeginInvoke()

    # 4. Main console thread Main messaging loop pump structure
    $Msg = New-Object DarkWin32Msg
    while (-not $AsyncResult.IsCompleted) {
        # Pump messages to process underlying WebView2 navigation frames
        if ([DarkComEngine]::PeekMessage([ref]$Msg, [IntPtr]::Zero, 0, 0, 1)) {
            [DarkComEngine]::TranslateMessage([ref]$Msg) | Out-Null
            [DarkComEngine]::DispatchMessage([ref]$Msg) | Out-Null
        }
        [System.Threading.Thread]::Sleep(10)
    }

    # Harvest output string from Runspace pipeline pool boundaries
    $ResponsePayload = $PowerShellPipeline.EndInvoke($AsyncResult)

    if ($ResponsePayload) {
        Write-Host ""
        Write-Host "$ESC[92m[RESPONSE DATA]:$ESC[0m"
        return $ResponsePayload
    } else {
        Write-Error "$ESC[91m[ERROR] Headless browser execution failed or returned an invalid null string object parameter reference.$ESC[0m"
    }
}
catch {
    Write-Error "$ESC[91m[FATAL] An error crashed the processing loop: $_$ESC[0m"
}
finally {
    # Perform clean component layout destruction
    if ($PowerShellPipeline) { $PowerShellPipeline.Dispose() }
    if ($Runspace) { $Runspace.Close(); $Runspace.Dispose() }
}
