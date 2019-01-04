#include "Patterns.h"
#include "LEDs.h"

// The current pattern
void (*currentPattern)() = pattern_solidColor;

/**
 * Cycle patterns
 */
std::vector<void (*)()> cyclingPatterns = {pattern_waves, pattern_noise, pattern_rainbow, pattern_rainbowWithGlitter, pattern_solidColor};
// Index number of which pattern is current
uint8_t currentCyclePatternIndex = 0;

void pattern_cycle() {
    if (cyclingPatterns.size() == 0) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else if (currentCyclePatternIndex >= cyclingPatterns.size()) {
        currentCyclePatternIndex = 0;
    }

	cyclingPatterns[currentCyclePatternIndex]();

	EVERY_N_SECONDS(10) {
		currentCyclePatternIndex = (currentCyclePatternIndex + 1) % cyclingPatterns.size();
	}
}


/**
 * Rainbow pattern
 */
void pattern_rainbow() {
	CHSV hsv;
	hsv.val = 255;
	hsv.sat = 240;
	for (int led = 0; led < NUM_LEDS; led++) {
		hsv.hue = gHue + gHueFraction + getPosX(led) * 7 + getPosY(led) * 7;
		leds[led] = hsv;
	}
}

/**
 * Rainbow pattern with glitter
 */
void pattern_rainbowWithGlitter() {
	pattern_rainbow();

	if (random8() < 80) {
		leds[random16(NUM_LEDS)] += CRGB::White;
	}
}

/**
 * Rainbow pattern
 */
void pattern_noise() {
	CHSV hsv;
	hsv.val = 255;
	hsv.sat = 240;
	for (int led = 0; led < NUM_LEDS; led++) {
		hsv.hue = inoise8(getPosX(led) << 5, getPosY(led) << 5, millis() / 10);
		leds[led] = hsv;
	}
}

/**
 * Waves pattern
 */
void pattern_waves() {
    pattern_solidColor();
    for (uint8_t led = 0; led < NUM_LEDS; led++) {
        float dx = getPosX(led) - (NUM_COLS - 1)/2.0;
        float dy = getPosY(led) - (NUM_ROWS - 1)/2.0;
        float distanceFromCenter = sqrt(pow(dx, 2) + pow(dy, 2));

        fadeToBlackBy(leds + led, 1, sin8(distanceFromCenter * 50 - (gHue + gHueFraction) * 2) * 0.8);
    }
}

/**
 * Confetti pattern
 * random colored speckles that blink in and fade smoothly
 */
void pattern_confetti() {
	fadeToBlackBy(leds, NUM_LEDS, 5);

	int pos = random16(NUM_LEDS);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);
}


/**
 * Sinelon pattern
 * a colored dot sweeping back and forth, with fading trails
 */
void pattern_sinelon() {
	fadeToBlackBy(leds, NUM_LEDS, 20);
	int pos = beatsin16(13, 0, NUM_LEDS);
	leds[pos] += CHSV(gHue, 255, 192);
}


/**
 * BPM pattern
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
void pattern_bpm() {
	uint8_t BeatsPerMinute = 62;
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
	for (int i = 0; i < NUM_LEDS; i++) { //9948
		leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
	}
}


/**
 * Juggle pattern
 * eight colored dots, weaving in and out of sync with each other
 */
void pattern_juggle() {
	fadeToBlackBy(leds, NUM_LEDS, 20);
	byte dothue = 0;
	for (int i = 0; i < 8; i++) {
		leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
		dothue += 32;
	}
}

/**
 * Flashing pattern
 * 
 * Configuration options:
 * - flashingConfig: The flashing pattern
 */
uint8_t flashCounter = 0;
typedef enum FlashingState {
	FlashingStateOn, FlashingStateOff, FlashingStateDelay
} FlashingState;
FlashingState flashingState = FlashingStateOff;
FlashingConfiguration flashingConfig = {
		.onTime = 50,
		.offTime = 50,
		.repetitions = 2,
		.delayTime = 100
};
void pattern_flashing() {
	static CEveryNMillis flashOnTimer;
	static CEveryNMillis flashOffTimer;
	static CEveryNMillis flashDelayTimer;

	flashOnTimer.setPeriod(flashingConfig.onTime);
	flashOffTimer.setPeriod(flashingConfig.offTime);
	flashDelayTimer.setPeriod(flashingConfig.delayTime);

	switch (flashingState) {
	case FlashingStateOn:
		if (flashOnTimer) {
			fill_solid(leds, NUM_LEDS, CRGB::Black);

			if (flashCounter >= flashingConfig.repetitions) {
				flashingState = FlashingStateDelay;
				flashDelayTimer.reset();
			} else {
				flashingState = FlashingStateOff;
				flashOffTimer.reset();
			}
		}
		break;
	case FlashingStateOff:
		if (flashOffTimer) {
			flashCounter++;
			fill_solid(leds, NUM_LEDS, solidLEDColor);
			flashingState = FlashingStateOn;
			flashOnTimer.reset();
		}
		break;
	case FlashingStateDelay:
		if (flashDelayTimer) {
			flashCounter = 1;

			fill_solid(leds, NUM_LEDS, solidLEDColor);
			flashingState = FlashingStateOn;
			flashOnTimer.reset();
		}
		break;
	}
}

/**
 * Solid color pattern
 * 
 * Configuration options:
 * - solidLEDColor: The solid color
 */
CRGB solidLEDColor = CRGB::Cyan;
void pattern_solidColor() {
	fill_solid(leds, NUM_LEDS, solidLEDColor);
}