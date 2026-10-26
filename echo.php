<?php
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
header('Access-Control-Allow-Headers: Content-Type, X-Requested-With, Authorization');

// Handle preflight OPTIONS request gracefully
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

if (session_status() === PHP_SESSION_NONE) {
    @session_start();
}

// 1. Session Persistence Tracker
if (!isset($_SESSION['echo_test_hits'])) {
    $_SESSION['echo_test_hits'] = 1;
} else {
    $_SESSION['echo_test_hits']++;
}

// 2. Application-Wide Persistence Tracker (Simulating Application Vars)
$appFile = __DIR__ . '/app_memory.json';
$appHits = 1;

if (file_exists($appFile)) {
    $appContent = @file_get_contents($appFile);
    $appData = $appContent ? json_decode($appContent, true) : null;
    if (is_array($appData) && isset($appData['global_hits'])) {
        $appHits = intval($appData['global_hits']) + 1;
    }
}
@file_put_contents($appFile, json_encode(['global_hits' => $appHits]));

// 3. Inbound Stream Captures
$raw_input = file_get_contents('php://input');
$json_decoded = json_decode($raw_input, true);

// Safely collect headers across different Apache/PHP configurations without throwing errors
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

// 4. Construct Telemetry Output
$telemetry = [
    "status" => "success",
    "engine" => "DarkCom2 Dynamic Verification Probe",
    "timestamp" => date('Y-m-d H:i:s'),
    "session_context" => [
        "session_id" => session_id(),
        "hit_counter" => $_SESSION['echo_test_hits']
    ],
    "application_context" => [
        "scope" => "Global System Tracking",
        "global_counter" => $appHits
    ],
    "request_details" => [
        "method" => $_SERVER['REQUEST_METHOD'] ?? 'GET',
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