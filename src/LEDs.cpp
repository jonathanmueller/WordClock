#define FASTLED_INTERNAL  // Disable pragma message

#include "LEDs.h"

#include <algorithm>
#include <array>
#include <map>

#include "Apps.h"
#include "Configuration.h"
#include "WebCommandHandler.h"

FASTLED_USING_NAMESPACE

/*
 Matrix display is wired as following:

        0 >	1 >	2 >	3 >	4 >	5 >	6 >	7 >	8 >	9 >	10
                                                 V
        21< 20< 19<	18<	17<	16<	15<	14<	13<	12<	11
        V
        22>	23>	24>	25>	26>	27>	28>	29>	30>	31>	32
                                                V
        ...
 */

CRGB * const leds_main = new CRGB[NUM_LEDS];
CRGB * const leds_buffer = new CRGB[NUM_LEDS];

CRGB *leds = leds_main;

uint8_t wordPositions[][2] PROGMEM = {
    {0, 2} /* ES    */, {3, 3} /* IST     */, {7, 4} /* FÜNF   */, {18, 4} /* ZEHN  */, {11, 7} /* ZWANZIG */, {22, 4} /* DREI  */, {26, 7} /* VIERTEL */, {38, 4} /* NACH  */, {35, 3} /* VOR     */, {44, 4} /* HALB  */, {49, 5} /* ZWÖLF   */, {62, 4} /* ZWEI  */, {61, 3} /* EIN    */, {60, 4} /* EINS    */, {55, 6} /* SIEBEN */, {67, 4} /* DREI  */, {73, 4} /* FÜNF    */, {85, 3} /* ELF   */, {81, 4} /* NEUN    */, {77, 4} /* VIER   */, {89, 4} /* ACHT  */, {93, 4} /* ZEHN    */, {104, 5} /* SECHS */, {99, 3} /* UHR     */
};

uint8_t glyphs[][NUM_ROWS * NUM_COLS] PROGMEM = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 109, 0, 0, 0, 0, 0, 0, 0, 0, 73, 255, 0, 0, 0, 0, 0, 0, 0, 0, 109, 255, 0, 0, 0, 0, 0, 0, 0, 0, 109, 255, 0, 0, 0, 0, 0, 0, 0, 0, 73, 255, 0, 0, 0, 0, 146, 255, 36, 0, 73, 255, 36, 0, 0, 0, 0, 146, 255, 146, 36, 255, 146, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 109, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 73, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 73, 0, 0, 0, 0, 0, 73, 255, 0, 0, 255, 255, 182, 0, 0, 0, 182, 255, 255, 219, 0, 219, 255, 255, 109, 0, 255, 255, 255, 182, 0, 0, 0, 255, 255, 255, 255, 255, 255, 109, 0, 0, 0, 0, 109, 255, 255, 255, 255, 73, 0, 0, 0, 0, 0, 73, 255, 255, 255, 255, 36, 0, 0, 0, 0, 109, 255, 255, 255, 255, 255, 255, 73, 0, 0, 146, 255, 255, 255, 73, 255, 255, 255, 255, 73, 0, 255, 255, 255, 36, 0, 0, 255, 255, 182, 0, 0, 36, 255, 36, 0, 0, 0, 0, 0, 0, 0, 0},
    //{ 0,0,0,146,255,255,255,146,0,0,0,0,73,255,255,219,182,219,255,255,73,0,0,255,255,255,109,0,109,255,255,255,36,182,255,255,255,146,0,146,255,255,255,182,255,255,255,255,219,0,219,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,182,255,255,255,255,182,255,255,255,255,182,0,255,255,255,146,0,146,255,255,255,36,0,73,255,255,255,182,255,255,255,73,0,0,0,0,146,255,255,255,146,0,0,0 }
};

std::array<bool, NUM_MATRIX_LEDS> getEnabledLEDsForWords(const std::vector<uint8_t> &wordArray) {
    std::array<bool, NUM_MATRIX_LEDS> enabledLEDs;
    enabledLEDs.fill(false);

    for (uint8_t currentWord : wordArray) {
        uint8_t start = pgm_read_byte(&(wordPositions[currentWord][0]));
        uint8_t length = pgm_read_byte(&(wordPositions[currentWord][1]));

        for (uint8_t k = 0; k < length; k++) {
            enabledLEDs[start + k] = true;
        }
    }
    return enabledLEDs;
}

void showWords(const std::vector<uint8_t> &wordArray, const std::vector<uint8_t> &wordArray2, float fadePercent) {
    auto enabledLEDsFrom = getEnabledLEDsForWords(wordArray);
    auto enabledLEDsTo = getEnabledLEDsForWords(wordArray2);

    /* Fade the LEDs */
    for (uint8_t i = 0; i < NUM_MATRIX_LEDS; i++) {
        leds[i].nscale8_video(dim8_video(255 * (fadePercent * (enabledLEDsTo[i] ? 1 : 0) + (1 - fadePercent) * (enabledLEDsFrom[i] ? 1 : 0))));
    }
}

void showGlyph(Glyph glyph) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint8_t x = getPosX(i);
        uint8_t y = getPosY(i);
        if (x >= NUM_COLS || y >= NUM_ROWS) {
            leds[i].nscale8(0);
            continue;
        }
        leds[i].nscale8(dim8_video(pgm_read_byte(&(glyphs[glyph][x + y * NUM_COLS]))));
    }
}

uint8_t getPosX(uint8_t led) {
    if (led >= NUM_MATRIX_LEDS) {
        return -1;
    }

    // LED is matrix LED

    uint8_t y = (led / NUM_COLS);
    if (y % 2 == 0) {
        return (led % NUM_COLS);
    } else {
        return NUM_COLS - 1 - (led % NUM_COLS);
    }
}

uint8_t getPosY(uint8_t led) {
    if (led >= NUM_MATRIX_LEDS) {
        return -1;
    }

    // LED is matrix LED

    return (led / NUM_COLS);
}

uint8_t gHue = 0;
float gHueFraction = 0;

// Setup / Loop
void setupLEDs() {
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(configuration.brightness);

    /* Load from saved config */
    executeCommand("display pattern " + String(configuration.pattern));
    executeCommand("display color " + String(configuration.color));
    executeCommand("display show " + String(configuration.app));
}

uint8_t brightnessFadeSpeed = 1;
void updateLEDs() {
    /* Fade brightness */
    sint16_t diff = (sint16_t)configuration.brightness - (sint16_t)FastLED.getBrightness();
    if (diff != 0) {
        if (brightnessFadeSpeed == 0 || (diff < (sint16_t)brightnessFadeSpeed && diff > -(sint16_t)brightnessFadeSpeed)) {
            FastLED.setBrightness(configuration.brightness);
        } else if (diff > 0) {
            FastLED.setBrightness(min(255, FastLED.getBrightness() + brightnessFadeSpeed));
        } else if (diff < 0) {
            FastLED.setBrightness(max(0, FastLED.getBrightness() - brightnessFadeSpeed));
        }
    }

    // Call the current pattern function once, updating the 'leds' array
    showCurrentApp();

    FastLED.show();
    
    EVERY_N_MILLISECONDS_I(hueCounter, 20) {
        gHue++;
    }
    gHueFraction = (float)hueCounter.getElapsed() / hueCounter.getPeriod();

}