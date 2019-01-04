#include "Firmware.h"

#include "LEDs.h"
#include "Network.h"
#include "TimeKeeper.h"
#include "TemperatureKeeper.h"
#include "WebServer.h"
#include "Configuration.h"
#include "ButtonHandler.h"
#include "SerialHandler.h"
#include "UpdateHandler.h"

void setup() {
	Serial.begin(115200);
	Serial.println();
	ESP.wdtEnable(500);

	setupButtonHandler();
	setupSerialHandler();
	setupConfiguration();
	setupLEDs();
	setupNetwork();
	setupWebServer();
	setupUpdateHandler();
	setupTimeKeeper();
	// setupTemperatureKeeper();
}

void loop() {
	if (Update.isRunning()) { return; }
	rebootIfNecessary();
	
	updateWebServer();
	updateSerialHandler();
	updateTimeKeeper();
	// updateTemperatureKeeper();
	updateButtonHandler();

	EVERY_N_MILLIS(1000/FRAMES_PER_SECOND) {
		updateLEDs();
	}
}

