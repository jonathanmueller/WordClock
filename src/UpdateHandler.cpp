#include "UpdateHandler.h"
#include "WebServer.h"
#include "LEDs.h"
#include "Apps.h"

boolean shouldReboot = false;

float updateStatus = 0.0;

void displayUpdateStatus() {
	if (!Update.hasError()) {
        if (Update.isFinished()) {
            fill_solid(leds, NUM_LEDS, CRGB::Green);
			showGlyph(Glyph::Checkmark);
        } else {
            fill_solid(leds, NUM_LEDS, CRGB::Yellow);
            float fract;
            float RADIUS = sqrt((float)NUM_COLS*NUM_COLS/4 + (float)NUM_ROWS*NUM_ROWS/4);
            for (uint8_t led = 0; led < NUM_LEDS; led++) {
                float dx = getPosX(led) - (NUM_COLS - 1)/2.0;
                float dy = getPosY(led) - (NUM_ROWS - 1)/2.0;
                float distanceFromCenter = sqrt(pow(dx, 2) + pow(dy, 2));

                fract = RADIUS * updateStatus - distanceFromCenter;
                fract = _min(_max(fract, 0), 1);
                nscale8_video(leds + led, 1, 255 * fract);
                fade_video(leds + led, 1, dim8_video(sin8(distanceFromCenter * 50 - (gHue + gHueFraction) * 30) * 0.8));
            }
        }
	} else {
		fill_solid(leds, NUM_LEDS, CRGB::Red);
		showGlyph(Glyph::Cross);
	}
}


#define PROGRESS_BAR_WIDTH 60
#define PROGRESS_BAR_FPS 10

unsigned int lastPrintedProgressBar = 0;

void printUpdateProgressBar() {
	if (lastPrintedProgressBar > millis() - (1000/(PROGRESS_BAR_FPS))) {
		return;
	}
	lastPrintedProgressBar = millis();

	int progressWidth = updateStatus * PROGRESS_BAR_WIDTH;

	Serial.print("\r[");
	for (uint8_t x = 0; x < PROGRESS_BAR_WIDTH; x++) {
		Serial.print(x < progressWidth ? '=' : (x == progressWidth ? '>' : ' '));
	}
	Serial.printf("] %.1f%%", updateStatus * 100);
}

void uploadHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
	updateStatus = (float)(index+len) / request->contentLength();
	if (!index) {
		// Get desired sector
		int command = U_FLASH;
		if (request->url() == "/updateSPIFFS") {
			command = U_SPIFFS;
		}

		currentApp = displayUpdateStatus;

		SPIFFS.end();

		Serial.printf("Update Start: %s\n", filename.c_str());

		Update.runAsync(true);
		if (!Update.begin(request->contentLength(), command)) {
			Update.printError(Serial);
		}
	}
	if (!Update.hasError()) {
		if (Update.write(data, len) != len) {
			Update.printError(Serial);
		}
	}
	
	printUpdateProgressBar();

	if (final) {
		Serial.println();
		if (Update.end(true)) {
			Serial.printf("Update Success: %uB\n", index + len);
		} else {
			Update.printError(Serial);
		}
	}
	
	updateLEDs();
}

void requestHandler(AsyncWebServerRequest * request) {
	shouldReboot = !Update.hasError();

	AsyncWebServerResponse *response = request->beginResponse(200, F("application/json"), shouldReboot ? F("{\"success\":true}") : F("{\"success\":false}"));
	response->addHeader("Connection", "close");
	request->send(response);
}

void rebootIfNecessary() {
	if (shouldReboot) {
		Serial.println("Rebooting...");
		FastLED.show();
		delay(100);
		ESP.restart();
	}
}

void setupUpdateHandler() {
	// Simple Firmware Update Form
	server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
		request->send(200, "text/html", F("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='image'><input type='submit' value='Update'></form><br><form method='POST' action='/updateSPIFFS' enctype='multipart/form-data'><input type='file' name='image'><input type='submit' value='Update SPIFFS'></form>"));
	});
	server.on("/update", HTTP_POST, requestHandler, uploadHandler);
	server.on("/updateSPIFFS", HTTP_POST, requestHandler, uploadHandler);
}
