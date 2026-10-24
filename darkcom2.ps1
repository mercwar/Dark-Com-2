<#
============================================================================
GATEWAY CONTROL CONSOLE - POWERSHELL FUNCTION LIBRARY (MODERNIZED)
COMPILED FROM ECLIPSE CONTROL PANEL (AVIS-2026 PROTOCOL)
TARGET ENDPOINT: https://httpbin.org
============================================================================
#>

$global:Url = "https://httpbin.org"

function Invoke-EclipseGatewayTest {
    [CmdletBinding()]
    param()
    process {
        Write-Host ""
        Write-Host " [~] Starting Eclipse Version 2.0 (AVIS-2026)..." -ForegroundColor Cyan
        Write-Host ""
        
        $RequestUrl = "$global:Url/get?public=true&application=true&write=false"
        try {
            $Response = Invoke-RestMethod -Uri $RequestUrl -Method Get -TimeoutSec 10
            Write-Host " [+] The Eclipse Gateway is ALIVE, Loading..." -ForegroundColor Green
            Start-Sleep -Seconds 1
            return $Response
        }
        catch {
            Write-Error "[-] Gateway connectivity failure: $_"
            return $null
        }
    }
}

function Invoke-GatewayRequest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("GET", "POST", "PUT", "DELETE")]
        [string]$Method,
        
        [Parameter(Mandatory = $true)]
        [string]$Path,
        
        [Parameter(Mandatory = $false)]
        [hashtable]$Headers = @{},
        
        [Parameter(Mandatory = $false)]
        [string]$Body
    )
    process {
        if (-not $Path.StartsWith("/")) { $Path = "/" + $Path }
        $TargetUrl = "$global:Url$Path"
        
        $Params = @{
            Uri         = $TargetUrl
            Method      = $Method
            Headers     = $Headers
            TimeoutSec  = 15
            ErrorAction = "Stop"
        }
        
        if ($Body -and $Method -in @("POST", "PUT", "DELETE")) {
            $Params.Body = $Body
            if (-not $Headers.ContainsKey("Content-Type")) {
                $Params.ContentType = "application/json"
            }
        }
        
        try {
            return Invoke-RestMethod @Params
        }
        catch {
            Write-Error "[-] Request failed to $TargetUrl : $_"
            return $null
        }
    }
}

function ConvertTo-CustomHeaders {
    param([string]$HeaderString)
    $Headers = @{}
    if (-not [string]::IsNullOrWhiteSpace($HeaderString)) {
        # Format: Key1=Value1;Key2=Value2
        $pairs = $HeaderString -split '[;,]'
        foreach ($pair in $pairs) {
            if ($pair -match '(.+?)[=:]\s*(.+)') {
                $Headers[$Matches[1].Trim()] = $Matches[2].Trim()
            }
        }
    }
    return $Headers
}

function Show-GatewayMenu {
    param(
        [string]$Message = ""
    )
    Clear-Host
    Write-Host "====================================================" -ForegroundColor Yellow
    Write-Host "     GATEWAY CONTROL CONSOLE (AVIS-2026 v2.0)       " -ForegroundColor Yellow
    Write-Host "====================================================" -ForegroundColor Yellow
    Write-Host " Target Endpoint: $global:Url" -ForegroundColor Gray
    Write-Host "----------------------------------------------------"
    Write-Host " [1] Run Gateway Connectivity Test (GET /get)"
    Write-Host " [2] Send Custom GET Request"
    Write-Host " [3] Send Custom POST Request (with Headers/Body)"
    Write-Host " [4] Send Custom PUT Request (with Headers/Body)"
    Write-Host " [5] Send Custom DELETE Request"
    Write-Host " [6] Export Last Response to JSON File"
    Write-Host " [7] Exit Console"
    Write-Host "----------------------------------------------------"
    if ($Message) {
        Write-Host " $Message" -ForegroundColor Magenta
        Write-Host "----------------------------------------------------"
    }
}

function Start-ConsoleInterface {
    $global:LastResponse = $null

    # Run initial verification sequence
    $InitialCheck = Invoke-EclipseGatewayTest
    if (-not $InitialCheck) {
        Write-Host "[-] Initial health check failed. Proceeding to menu anyway..." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
    }
    
    $Running = $true
    $MenuMessage = ""
    
    while ($Running) {
        Show-GatewayMenu -Message $MenuMessage
        $MenuMessage = "" # Clear message for next loop
        
        $Choice = Read-Host "Select an option [1-7]"
        switch ($Choice) {
            "1" {
                Clear-Host
                Write-Host "Executing Gateway Connectivity Test..." -ForegroundColor Cyan
                $Result = Invoke-EclipseGatewayTest
                if ($Result) {
                    $global:LastResponse = $Result
                    Write-Host "`n[Response Received]:" -ForegroundColor Green
                    $Result | Format-List * | Out-String | Write-Host -ForegroundColor White
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "2" {
                Clear-Host
                Write-Host "--- Custom GET Request ---" -ForegroundColor Cyan
                $Path = Read-Host "Enter endpoint path (e.g., /get or /ip)"
                $HeaderInput = Read-Host "Enter custom headers (optional, e.g., 'Authorization=Bearer token')"
                $Headers = ConvertTo-CustomHeaders -HeaderString $HeaderInput
                
                Write-Host "`nSending GET request..." -ForegroundColor Gray
                $Result = Invoke-GatewayRequest -Method "GET" -Path $Path -Headers $Headers
                if ($Result) {
                    $global:LastResponse = $Result
                    Write-Host "`n[Response]:" -ForegroundColor Green
                    $Result | ConvertTo-Json -Depth 10 | Write-Host -ForegroundColor White
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "3" {
                Clear-Host
                Write-Host "--- Custom POST Request ---" -ForegroundColor Cyan
                $Path = Read-Host "Enter endpoint path (e.g., /post)"
                $HeaderInput = Read-Host "Enter custom headers (optional)"
                $Headers = ConvertTo-CustomHeaders -HeaderString $HeaderInput
                $Body = Read-Host "Enter JSON Body payload (e.g., {\"test\":\"value\"})"
                
                Write-Host "`nSending POST request..." -ForegroundColor Gray
                $Result = Invoke-GatewayRequest -Method "POST" -Path $Path -Headers $Headers -Body $Body
                if ($Result) {
                    $global:LastResponse = $Result
                    Write-Host "`n[Response]:" -ForegroundColor Green
                    $Result | ConvertTo-Json -Depth 10 | Write-Host -ForegroundColor White
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "4" {
                Clear-Host
                Write-Host "--- Custom PUT Request ---" -ForegroundColor Cyan
                $Path = Read-Host "Enter endpoint path (e.g., /put)"
                $HeaderInput = Read-Host "Enter custom headers (optional)"
                $Headers = ConvertTo-CustomHeaders -HeaderString $HeaderInput
                $Body = Read-Host "Enter JSON Body payload"
                
                Write-Host "`nSending PUT request..." -ForegroundColor Gray
                $Result = Invoke-GatewayRequest -Method "PUT" -Path $Path -Headers $Headers -Body $Body
                if ($Result) {
                    $global:LastResponse = $Result
                    Write-Host "`n[Response]:" -ForegroundColor Green
                    $Result | ConvertTo-Json -Depth 10 | Write-Host -ForegroundColor White
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "5" {
                Clear-Host
                Write-Host "--- Custom DELETE Request ---" -ForegroundColor Cyan
                $Path = Read-Host "Enter endpoint path (e.g., /delete)"
                $HeaderInput = Read-Host "Enter custom headers (optional)"
                $Headers = ConvertTo-CustomHeaders -HeaderString $HeaderInput
                
                Write-Host "`nSending DELETE request..." -ForegroundColor Gray
                $Result = Invoke-GatewayRequest -Method "DELETE" -Path $Path -Headers $Headers
                if ($Result) {
                    $global:LastResponse = $Result
                    Write-Host "`n[Response]:" -ForegroundColor Green
                    $Result | ConvertTo-Json -Depth 10 | Write-Host -ForegroundColor White
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "6" {
                Clear-Host
                Write-Host "--- Export Last Response to JSON ---" -ForegroundColor Cyan
                if ($null -eq $global:LastResponse) {
                    Write-Host "[-] No response recorded yet. Run a request first." -ForegroundColor Yellow
                } else {
                    $DefaultPath = "$PWD\gateway_response_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
                    $FilePath = Read-Host "Enter file path to save [$DefaultPath]"
                    if ([string]::IsNullOrWhiteSpace($FilePath)) {
                        $FilePath = $DefaultPath
                    }
                    try {
                        $global:LastResponse | ConvertTo-Json -Depth 10 | Out-File -FilePath $FilePath -Encoding utf8
                        Write-Host "[+] Successfully exported response to: $FilePath" -ForegroundColor Green
                    }
                    catch {
                        Write-Error "[-] Failed to save file: $_"
                    }
                }
                Read-Host "`nPress Enter to return to menu"
            }
            "7" {
                Write-Host "`nExiting Gateway Control Console. Goodbye." -ForegroundColor Yellow
                $Running = $false
            }
            Default {
                $MenuMessage = "Invalid choice: Please select a valid number from 1 to 7."
            }
        }
    }
}

# Launch the application
Start-ConsoleInterface