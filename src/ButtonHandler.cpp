#include "ButtonHandler.h"
#include "Configuration.h"
#include "Network.h"
#include "LEDs.h"

void setupButtonHandler() {
    pinMode(BUTTON_PIN, INPUT);
}

void updateButtonHandler() {
    static uint8_t buttonHoldCounter = 0;
    static bool heldButton = false;
    static bool lastButtonState = false;
    static bool buttonStillPressedSinceBoot = true;

    bool buttonState = (!digitalRead(BUTTON_PIN)) && !buttonStillPressedSinceBoot;

	if (buttonState) {
        // Toggle SoftAP if soft ap button is pressed for at least 5 seconds
        EVERY_N_MILLIS(100) {
            // Count up every 100ms
            buttonHoldCounter++;
            if (buttonHoldCounter == RESET_BUTTON_DURATION) {
                heldButton = true;

                Serial.println("Resetting...");

                delay(100);
                factoryReset();
            }
            if (buttonHoldCounter > RESET_BUTTON_DURATION) {
                buttonHoldCounter = RESET_BUTTON_DURATION;
            }
        }
    } else {
		if (lastButtonState) {
            // Button was released
			if (!heldButton) {
                // Button was pressed, but not held
                switchNetworkMode();
			}
		}

        // Reset hold counter
		heldButton = false;
		buttonHoldCounter = 0;
	}
    
    if (digitalRead(BUTTON_PIN)) { buttonStillPressedSinceBoot = false; }
    lastButtonState = buttonState;
}