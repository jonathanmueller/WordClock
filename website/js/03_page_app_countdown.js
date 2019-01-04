function setCountdown() {
    app.dialog.prompt("Bitte gib die Zeit ein, von der heruntergezählt werden soll:", "Zeit eingeben", function(text) {
        // TODO: calculate seconds from format
        console.log("Setting time");
    });
}