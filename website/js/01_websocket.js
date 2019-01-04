// WebSocket methods
var wsUrl = 'ws://' + window.location.host + '/ws';
//wsUrl = 'ws://ledclock.local/ws';

var socket;
var currentWebSocketHandler = defaultWebSocketHandler;
var isConnected = false;
var hideIndicatorOnMessage = false;
var queuedMessageCall = null;

function connect() {
    if (socket) {
        socket.close();
    }

    isConnected = false;
    hideIndicatorOnMessage = false;

    socket = new WebSocket(wsUrl);

/*
    //////////////////////////
    ////// DUMMY SOCKET //////
    //////////////////////////
    socket = {};
    // Dummy send: respond with success
    socket.send  = function() { setTimeout(function() { socket.onmessage({data: '{"success":true,"time":"2018-05-18T01:23:45.678Z","settings":{"brightness":85}}'}); }, 200); };
    // Dummy close: call socket.onclose
    socket.close = function() { setTimeout(function() { socket.onclose(); }, 100); };
    // Call open callback
    setTimeout(function() { socket.onopen(); }, 200);
    // Trigger dummy welcome message by sending an empty message
    socket.send("");
    //////////////////////////
    //// END DUMMY SOCKET ////
    //////////////////////////
*/

    socket.onerror = function(error) {
        console.log('WebSocket Error: ', error);
    };

    socket.onopen = function(event) {
        isConnected = true;
        currentWebSocketHandler = handleWelcomeMessage;
        console.log("WebSocket Connected");
    };

    socket.onclose = function(event) {
        app.preloader.hide();
        if (!isConnected) {
            $$("#loginError").removeClass("display-none").html("Keine Verbindung zum Gerät");
        }
        isConnected = false;


        console.log("WebSocket Disconnected", event);
        socket = null;
    };

    socket.onmessage = function(event) {
        if (hideIndicatorOnMessage) {
            hideIndicatorOnMessage = false;
            app.preloader.hide();
        }

        var response;
        try {
            response = JSON.parse(event.data);
        } catch (SyntaxError) {
            response = event.data;
        }
        var webSocketHandler = currentWebSocketHandler;
        currentWebSocketHandler = defaultWebSocketHandler;
        webSocketHandler(response);
    };
}

function sendMessage(message, responseHandler, showIndicator) {
    if (!socket) {
        app.preloader.show();
        connect();        
        // if ($$(".login-screen").css("display") === "none") {
        //     app.dialog.alert("Keine Verbindung zur Uhr", "Fehler");
        // }
    } else {
        if (!isConnected) {
            console.log("not connected yet, queuing message");
            queuedMessageCall = function() {sendMessage(message, responseHandler, showIndicator);};
            return;
        }
        if (!responseHandler) { responseHandler = defaultWebSocketHandler; }
        currentWebSocketHandler = responseHandler;
        if (showIndicator) {
            hideIndicatorOnMessage = true;
            app.preloader.show();
        }
        console.log("sending ", message);
        socket.send(message);
    }
}

function wsCommand(command, successHandler) {
    sendMessage(command, function(response) {
        if (!response.success) {
            app.dialog.alert("Aktion konnte nicht ausgeführt werden.", "Fehler");
        } else if (successHandler) {
            successHandler(response);
        }
    }, true);
}

function handleCommandResponse(response, successHandler) {
    if (!response.success) {
        app.dialog.alert("Aktion konnte nicht ausgeführt werden.", "Fehler");
    } else if (successHandler) {
        successHandler();
    }
}

function defaultWebSocketHandler(response) {
    console.log("WebSocket Response: ", response);
}

function handleWelcomeMessage(message) {
    console.log("Got welcome message");
    sendMessage(localStorage.getItem('password'), handlePasswordResponse);
}

function handlePasswordResponse(response) {
    app.preloader.hide();
    if (!response.success) {
        localStorage.removeItem('passwordVerified');
        localStorage.removeItem('password');
        if (!loginScreen.opened) {
            loginScreen.open();
        } else {
            loginScreen.$el.find("input[type=password]")[0].select();
            var shakingElement = loginScreen.$el.find(".shaking-element");
            shakingElement.addClass("invalid-password");
            setTimeout(function() { shakingElement.removeClass("invalid-password"); }, 700);
        }
    } else {
        localStorage.setItem('passwordVerified', true);
        loginScreen.close();
        if (queuedMessageCall) {
            queuedMessageCall();
            queuedMessageCall = null;
        }
    }
}