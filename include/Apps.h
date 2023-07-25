#pragma once

#include <Arduino.h>
#define FASTLED_INTERNAL
#undef delay
#include <FastLED.h>

#include <vector>

// The current app
extern void (*currentApp)();
extern unsigned long appSwitchedAt;
extern CEveryNSeconds appCycleTimer;

void showCurrentApp();

// Cycle app
extern std::vector<void (*)()> cyclingApps;
void app_cycle();

void app_time();
void app_solid();
