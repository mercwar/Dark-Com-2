<?php
// Set headers for JSON inspection
header('Content-Type: application/json');

if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

// Gather comprehensive session telemetry and state
$dump_data = [
    "status" => "success",
    "server_time" => date('Y-m-d H:i:s'),
    "session_id" => session_id(),
    "session_name" => session_name(),
    "session_status" => session_status(),
    "cookie_params" => session_get_cookie_params(),
    "request_method" => $_SERVER['REQUEST_METHOD'],
    "total_session_keys" => count($_SESSION),
    "session_data" => $_SESSION ?? []
];

// Output pretty-printed JSON representation of the entire session
echo json_encode($dump_data, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
exit;
?>