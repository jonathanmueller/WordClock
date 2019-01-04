function setDisplayText() {
    app.dialog.prompt("Bitte gib den Text ein, der angezeigt werden soll:", "Text", function(text) {
        wsCommand("display show text " + text);
    });
}