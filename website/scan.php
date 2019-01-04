<?php
$ssids = array();

$ssids[] = array(
    "rssi" => -55,
    "ssid" => "LiSte24",
    "bssid" => "34:31:C4:CD:0B:90",
    "channel" => 6,
    "secure" => 4,
    "hidden" => false
);

$ssids[] = array(
    "rssi" => -86,
    "ssid" => "LiSte24",
    "bssid" => "CC:CE:1E:02:B8:E0",
    "channel" => 6,
    "secure" => 4,
    "hidden" => false
);

if (rand() % 3) {
    $ssids[] = array(
        "rssi"    => -88,
        "ssid"    => "devolo-30d32d30ea21",
        "bssid"   => "30:D3:2D:30:EA:21",
        "channel" => 6,
        "secure"  => 4,
        "hidden"  => false
    );
}

echo json_encode($ssids);