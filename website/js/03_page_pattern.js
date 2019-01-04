function setFlashingPattern() {
    app.dialog.prompt("Bitte gib das Blinkmuster ein.<p style='font-family: monospace;'>{ontime} {offtime} {repetitions} {delay}</p>", "Blinken", function(pattern) {
        pattern = pattern.trim();
        if (!pattern.match(/^(\d+ ){3}\d+$/)) {
            setFlashingPattern();
        } else {
            wsCommand("display pattern flashing " + pattern);
        }
    });
}