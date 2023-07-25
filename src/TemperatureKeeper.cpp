#include "TemperatureKeeper.h"

#include "DallasTemperature.h"

#define FASTLED_INTERNAL
#undef delay
#include <FastLED.h>

#include "OneWirePin16.h"

OneWirePin16 oneWirePin16;
DallasTemperature DS18B20(&oneWirePin16);

float currentTemperature = 0;

boolean temperatureSensorFound = true;

void setupTemperatureKeeper() {
    Serial.println(F("Setting up temperature keeper..."));
    DS18B20.begin();
    DS18B20.requestTemperatures();
    if (DS18B20.getTempCByIndex(0) == DEVICE_DISCONNECTED_C) {
        Serial.println(F("Disabling Temperature Module."));
        temperatureSensorFound = false;
    }
    DS18B20.setWaitForConversion(false);
}

void updateTemperatureKeeper() {
    if (!temperatureSensorFound) {
        return;
    }

    EVERY_N_SECONDS(TEMPERATURE_UPDATE_INTERVAL) {
        DS18B20.requestTemperatures();
    }
    EVERY_N_SECONDS_I(startUpdateTimer, 2) {  // 2 second delay to allow conversion
        startUpdateTimer.setPeriod(TEMPERATURE_UPDATE_INTERVAL);
        currentTemperature = DS18B20.getTempCByIndex(0);
    }
}
