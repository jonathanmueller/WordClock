function setPassword() {
    var pw1 = $$('#pw1').val();
    var pw2 = $$('#pw2').val();
    if (pw1 == "") {
        app.dialog.alert("Das Passwort darf nicht leer sein.", "Fehler");
    } else if (pw1 != pw2) {
        app.dialog.alert("Die Passwörter stimmen nicht überein.", "Fehler");
    } else {
        wsCommand("settings set password " + pw1, function(response) {
            localStorage.setItem("password", pw1);
            app.dialog.alert("Das Passwort wurde erfolgreich geändert!", "Passwort geändert")
            mainView.router.back();
        });
    }
}