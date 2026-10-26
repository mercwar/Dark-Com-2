<?php
// Set headers for JSON API response
header('Content-Type: application/json');

if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

/**
 * Unique stamp generator for the session sequence
 */
function get_unique_session_time_stamp() {
    if (!isset($_SESSION['eclipse_seq_counter'])) {
        $_SESSION['eclipse_seq_counter'] = 0;
    }
    $_SESSION['eclipse_seq_counter']++;
    return 't_' . $_SESSION['eclipse_seq_counter'] . '_' . hrtime(true);
}

/**
 * SET_DATA: Stores the 2-node entry deep inside $_SESSION using the preprocessed path array as keys
 */
function set_data($payload_data, $app = 'ECLIPSE', $type = 'json', $scope = 'private') {
    $session_id = session_id();

    // Build the preprocessed living path array
    $var_path = [
        $app,
        $scope, 
        $type,  
        date('Y'), 
        date('m'), 
        date('d'),
        date('H'), 
        date('i'), 
        date('s'),
        get_unique_session_time_stamp(),
        $session_id
    ];

    // The 2-Node Structure [Node 1: Living Address, Node 2: Original Payload]
    $two_node_entry = [
        'node_address' => $var_path,
        'payload' => $payload_data
    ];

    // Deep-assign into $_SESSION using path array segments as nested keys
    $ref = &$_SESSION;
    foreach ($var_path as $segment) {
        if (!isset($ref[$segment]) || !is_array($ref[$segment])) {
            $ref[$segment] = [];
        }
        $ref = &$ref[$segment];
    }
    $ref = $two_node_entry;
    unset($ref);

    return [
        "status" => "success",
        "session_id" => $session_id,
        "node_address" => $var_path,
        "entry" => $two_node_entry
    ];
}

/**
 * GET_DATA: Retrieves node data directly from $_SESSION using the node address path
 */
function get_data(array $node_address) {
    $current = $_SESSION;
    foreach ($node_address as $segment) {
        if (!is_array($current) || !array_key_exists($segment, $current)) {
            return null; // Node not found
        }
        $current = $current[$segment];
    }

    return $current;
}

/**
 * SNAPSHOT BACKUP: Bundles current session state into a downloadable JSON file
 */
function create_snapshot_backup() {
    $snapshot = [
        "timestamp" => date('Y-m-d H:i:s'),
        "session_id" => session_id(),
        "session_data" => $_SESSION ?? []
    ];

    $filename = "eclipse_snapshot_" . date('Ymd_His') . ".json";
    $filepath = sys_get_temp_dir() . '/' . $filename;
    
    file_put_contents($filepath, json_encode($snapshot, JSON_PRETTY_PRINT));

    return [
        "filename" => $filename,
        "filepath" => $filepath,
        "snapshot_data" => $snapshot
    ];
}

// ==========================================
// --- API REQUEST ROUTER ---
// ==========================================

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(["status" => "error", "message" => "Method not allowed. Use POST."]);
    exit;
}

$raw_input = file_get_contents('php://input');
$request_data = json_decode($raw_input, true);

$action = $request_data['action'] ?? 'set';

switch ($action) {
    case 'set':
        $payload = $request_data['payload'] ?? [];
        $app = $request_data['app'] ?? 'ECLIPSE';
        $type = $request_data['type'] ?? 'json';
        $scope = $request_data['scope'] ?? 'private';
        
        $result = set_data($payload, $app, $type, $scope);
        echo json_encode($result, JSON_PRETTY_PRINT);
        break;

    case 'get':
        $node_address = $request_data['node_address'] ?? [];
        
        $data = get_data($node_address);
        if ($data !== null) {
            echo json_encode(["status" => "success", "data" => $data], JSON_PRETTY_PRINT);
        } else {
            http_response_code(404);
            echo json_encode(["status" => "error", "message" => "Node address not found in session."], JSON_PRETTY_PRINT);
        }
        break;

    case 'snapshot':
        $backup = create_snapshot_backup();
        echo json_encode(["status" => "success", "backup_info" => $backup], JSON_PRETTY_PRINT);
        break;

    default:
        http_response_code(400);
        echo json_encode(["status" => "error", "message" => "Invalid action specified."]);
        break;
}
exit;
?>