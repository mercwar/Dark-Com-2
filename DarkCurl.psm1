# ====================================================================
# AIFVS-MODULE: DarkCurl.psm1
# PowerShell Command-Line Wrapper for DarkComHeadless WebView2 Engine
# ====================================================================

# Define the inline C-Sharp structural bridge to map the unmanaged DLL entry points cleanly
$NativeBridgeCode = @"
using System;
using System.Runtime.InteropServices;

public class DarkComEngine {
    // Explicit pointer mapping to your compiled pure C DLL function symbol
    [DllImport("DarkComHeadless.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
    public static extern IntPtr RunHeadlessRequest(string url, string postData);
}
"@

# Initialize the dynamic assembly type definitions inside the memory session space
if (-not ([System.Management.Automation.PSTypeName]'DarkComEngine').Type) {
    Add-Type -TypeDefinition $NativeBridgeCode -ErrorAction SilentlyContinue
}

function Invoke-DarkCurl {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true, Position = 0, HelpMessage = "Target URL endpoint string to transaction query.")]
        [string]$Url,

        [Parameter(Mandatory = $false, Position = 1, HelpMessage = "Optional raw data payload body configuration to process a POST request.")]
        [string]$Data = $null
    )

    process {
        Write-Verbose "[INFO] Routing transaction to headless browser container engine..."
        
        try {
            # Execute the pure C transaction allocation loop natively
            $ResultBufferPtr = [DarkComEngine]::RunHeadlessRequest($Url, $Data)

            if ($ResultBufferPtr -ne [IntPtr]::Zero) {
                # Marshal the returned unmanaged UTF-8 character byte pointer back into an organic PowerShell string object
                $ExtractedStringOutput = [Marshal]::PtrToStringAnsi($ResultBufferPtr)
                
                # Output matches raw curl text layout directly to standard out pipeline streams
                return $ExtractedStringOutput
            } else {
                Write-Error "[ERROR] Headless broker execution transaction returned an empty or invalid null buffer pointer."
            }
        }
        catch {
            Write-Error "[FATAL] Failed to handle Interop transaction parameters: $_"
        }
    }
}

# Explicitly export the cmdlet tool interface wrapper directly to the host terminal session context
Export-ModuleMember -Function Invoke-DarkCurl

# Register a command alias matching standard terminal conventions
Set-Alias -Name dcurl -Value Invoke-DarkCurl
