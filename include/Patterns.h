#pragma once

#include <Arduino.h>
#define FASTLED_INTERNAL
#undef delay
#include <FastLED.h>

#include <vector>

// The current pattern
extern void (*currentPattern)();
extern CEveryNSeconds patternCycleTimer;

// Cycling pattern
extern std::vector<void (*)()> cyclingPatterns;
void pattern_cycle();
void nextPattern();

// Patterns

void pattern_radar();
void pattern_rainbow();
void pattern_rainbowWithGlitter();
void pattern_noise();
void pattern_waves();
void pattern_confetti();
void pattern_sinelon();
void pattern_bpm();
void pattern_juggle();

/**
 * Flashing pattern
 *
 * Configuration options:
 * - flashingConfig: The flashing pattern
 */
typedef struct FlashingConfiguration {
    uint16_t onTime;
    uint16_t offTime;
    uint8_t repetitions;
    uint16_t delayTime;
} FlashingConfiguration;
extern FlashingConfiguration flashingConfig;
void pattern_flashing();

/**
 * Solid color pattern
 *
 * Configuration options:
 * - solidLEDColor: The solid color
 */
extern CRGB solidLEDColor;
void pattern_solidColor();