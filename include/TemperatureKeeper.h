#pragma once

#include <Arduino.h>
#include <RtcTemperature.h>

#define TEMPERATURE_UPDATE_INTERVAL 30

extern float currentTemperature;
extern boolean temperatureSensorFound;

void setupTemperatureKeeper();
void updateTemperatureKeeper();
