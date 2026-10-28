<?php
/**
 * ============================================================================
 *  DarkCom2 / AVIS-2026 Dynamic Verification Probe (tutorial.php)
 * ============================================================================
 */

// Clear any prior output buffers and disable error reporting output to keep JSON pristine
while (ob_get_level()) {
    ob_end_clean();
}
error_reporting(0);
ini_set('display_errors', '0');

// Set headers for clean JSON output, CORS compatibility, and cache prevention
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-cache, must-revalidate');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, DELETE');
header('Access-Control-Allow-Headers: Content-Type, X-Requested-With, Authorization, X-AVIS-Protocol');

// Handle preflight OPTIONS request gracefully
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

if (session_status() === PHP_SESSION_NONE) {
    @session_start();
}

// ============================================================================
// PROTOCOL CONSTANTS & $_application STATE MATRIX
// ============================================================================
define('SAVE_MODE_DISABLED', 0);
define('SAVE_MODE_TRANSIENT', 1);
define('SAVE_MODE_PERSISTENT', 2);

define('VALIDATE_MODE_LAX', 10);
define('VALIDATE_MODE_STRICT', 20);

// Initialize global application protocol structure
$GLOBALS['_application'] = [
    'ECLIPSE' => [
        'engine' => 'Eclipse Package Deployment Core',
        'version' => '1.0.0',
        'channel' => 'stable'
    ],
    'SESSION' => [
        'id' => session_id(),
        'hits' => 0
    ],
    'TELEMETRY' => [
        'client_ip' => $_SERVER['REMOTE_ADDR'] ?? 'UNKNOWN',
        'user_agent' => $_SERVER['HTTP_USER_AGENT'] ?? 'UNKNOWN',
        'request_time' => date('Y-m-d H:i:s')
    ],
    'SECURITY' => [
        'save_mode' => SAVE_MODE_PERSISTENT,
        'validate_mode' => VALIDATE_MODE_STRICT
    ]
];

// 1. Session Persistence Tracker
if (!isset($_SESSION['echo_test_hits'])) {
    $_SESSION['echo_test_hits'] = 1;
} else {
    $_SESSION['echo_test_hits']++;
}
$GLOBALS['_application']['SESSION']['hits'] = $_SESSION['echo_test_hits'];

// 2. Application-Wide Persistence Tracker (Simulating Application Vars)
$appFile = __DIR__ . '/app_memory.json';
$appHits = 1;

if ($GLOBALS['_application']['SECURITY']['save_mode'] === SAVE_MODE_PERSISTENT) {
    if (file_exists($appFile)) {
        $appContent = @file_get_contents($appFile);
        $appData = $appContent ? json_decode($appContent, true) : null;
        if (is_array($appData) && isset($appData['global_hits'])) {
            $appHits = intval($appData['global_hits']) + 1;
        }
    }
    @file_put_contents($appFile, json_encode(['global_hits' => $appHits], JSON_PRETTY_PRINT));
}

// 3. Inbound Stream Captures & Validation
$raw_input = file_get_contents('php://input');
$json_decoded = json_decode($raw_input, true);

if ($GLOBALS['_application']['SECURITY']['validate_mode'] === VALIDATE_MODE_STRICT) {
    // Perform strict protocol header checks if required
    $protocolHeader = $_SERVER['HTTP_X_AVIS_PROTOCOL'] ?? null;
}

// Safely collect headers across different Apache/PHP configurations
$collectedHeaders = [];
if (function_exists('getallheaders')) {
    $collectedHeaders = getallheaders();
} else {
    foreach ($_SERVER as $key => $value) {
        if (substr($key, 0, 5) === 'HTTP_') {
            $headerName = str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($key, 5)))));
            $collectedHeaders[$headerName] = $value;
        }
    }
}

// 4. Construct Full Telemetry Output Map
$telemetry = [
    "status" => "success",
    "protocol_signature" => "#AVIS::TUTORIAL_ECHO_V2",
    "timestamp" => date('Y-m-d H:i:s'),
    "application_state" => $GLOBALS['_application'],
    "request_details" => [
        "method" => $_SERVER['REQUEST_METHOD'] ?? 'GET',
        "uri" => $_SERVER['REQUEST_URI'] ?? '',
        "query_string" => $_SERVER['QUERY_STRING'] ?? '',
        "content_type" => $_SERVER['CONTENT_TYPE'] ?? ($_SERVER['HTTP_CONTENT_TYPE'] ?? 'Not Specified'),
        "content_length" => $_SERVER['CONTENT_LENGTH'] ?? strlen($raw_input)
    ],
    "payload_telemetry" => [
        "raw_stream" => $raw_input,
        "parsed_json" => $json_decoded
    ],
    "headers" => $collectedHeaders
];

echo json_encode($telemetry, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
exit;