// Init login page
var loginScreen = app.loginScreen.create({ el: $$(".login-screen") });

loginScreen.$el.find('#login-form').submit(function (e) {
    $$("#loginError").addClass("display-none").html("");
    e.preventDefault();
    e.stopPropagation();

    var password = loginScreen.$el.find('input[type=password]').val();
    localStorage.setItem('password', password);

    app.preloader.show();
    connect();
    return false;
});

loginScreen.$el.find("#login-button").on('click', function () { loginScreen.$el.find('#login-form').trigger('submit'); })

var resetClockMessage = "Wenn du das Passwort vergessen hast, musst du die Uhr zurücksetzen. Dabei werden alle Einstellungen gelöscht. Gib dazu bitte das Standardpasswort des WLAN Netzwerks der Uhr ein.";
var wrongPassFormatMessage = '<p><span class="color-red">Das Passwort besteht aus 10 alphanumerischen Zeichen</span></p>';
function resetClock(wrongFormat) {
    app.dialog.prompt(resetClockMessage + (wrongFormat ? wrongPassFormatMessage : ""), "Zurücksetzen", function(password) {
        if (!password.match(/^[A-Z0-9]{10}$/i)) {
            resetClock(true);
        } else {
            app.preloader.show();
            app.request.post("/reset", { password: password.toUpperCase() }, resetClockCallback, resetClockCallback);
        }
    });
}

function resetClockCallback(data) {
    app.preloader.hide();
    var error = false;
    if (typeof data !== "string") {
        error = true;
    } else {
        try {
            response = JSON.parse(data);
        } catch (SyntaxError) {
            error = true;
        }
        if (!response.success) { error = true; }
    }

    if (error) {
        app.dialog.alert("Die Uhr konnte nicht zurückgesetzt werden.", "Fehler");
    } else {
        app.dialog.alert("Die Uhr wurde erfolgreich zurückgesetzt. Bitte verbinde dich neu.", "Zurückgesetzt")
    }
}

if (!window.localStorage.getItem('password') || !window.localStorage.getItem('passwordVerified')) {
    loginScreen.open(false);
} else {
    app.preloader.show();
    connect();
}
