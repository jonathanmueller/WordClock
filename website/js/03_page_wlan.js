function wifiCallback(response) {
    app.dialog.alert("Netzwerkwechsel erfolgreich durchgeführt. Bitte neu verbinden.", "Erfolg");
    mainView.router.back({url:"/", force: true});
}

function connectToSSID() {
    var ssid = $$('#wifiSSID').val();
    var password = $$('#wifiPassword').val();
    wsCommand("wifi connect " + ssid + " " + password, wifiCallback);
}

function createSoftAP() {
    var ssid = $$('#softAPSSID').val();
    var password = $$('#softAPPassword').val();
    wsCommand("wifi softAP " + ssid + " " + password, wifiCallback);
}