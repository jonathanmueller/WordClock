#pragma once

#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>
#include <vector>

// The current app
extern void (*currentApp)();

// Cycle app
extern std::vector<void (*)()> cyclingApps;
void app_cycle();

extern uint16_t fadeTime;
void app_time();
void app_solid();
