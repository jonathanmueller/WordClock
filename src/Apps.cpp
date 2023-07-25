#include "Apps.h"

#include "Configuration.h"
#include "LEDs.h"
#include "Math.h"
#include "Patterns.h"
#include "TemperatureKeeper.h"
#include "TimeKeeper.h"

// The current app
void (*currentApp)() = app_time;
void (*previousApp)() = app_time;
CEveryNSeconds appCycleTimer(10);

// Cycle app
std::vector<void (*)()> cyclingApps = {app_time, app_solid};
uint8_t currentCycleAppIndex = 0;  // Index number of which app is current
unsigned long appSwitchedAt = 0;   // Time at which the app was switched, used for fading
void showCurrentApp() {
    currentApp();

    uint16_t millisecondsIntoApp = millis() - appSwitchedAt;
    float appFadePercent = (millisecondsIntoApp < configuration.fadeDuration) ? ((float)millisecondsIntoApp / configuration.fadeDuration) : 1;
    if (appFadePercent < 1) {
        leds = leds_buffer;
        void (*savedCurrentApp)() = currentApp;
        currentApp = previousApp;
        currentApp();
        currentApp = savedCurrentApp;
		leds = leds_main;

        fract8 fadeFract = appFadePercent * 255;

        for (uint16_t i = 0; i < NUM_LEDS; ++i) {
            leds[i] = leds_buffer[i].lerp8(leds[i], fadeFract);
        }
    } else {
        previousApp = currentApp;
    }
}

void app_cycle() {
    if (cyclingApps.size() == 0) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else if (currentCycleAppIndex >= cyclingApps.size()) {
        currentCycleAppIndex = 0;
    }

    cyclingApps[currentCycleAppIndex]();

    if (appCycleTimer) {
        previousApp = cyclingApps[currentCycleAppIndex];
        appSwitchedAt = millis();
        currentCycleAppIndex = (currentCycleAppIndex + 1) % cyclingApps.size();
        if (cyclingApps[currentCycleAppIndex] == app_solid) {
            nextPattern();
        }
    }
}

// Apps

std::vector<uint8_t> getWordsForTime(uint8_t hour, uint8_t minute) {
    if (minute >= 25) {
        hour++;
    }                  // offset for "5 vor halb X+1"
    hour = hour % 12;  // 24h -> 12h

    std::vector<uint8_t> words{ES, IST};

    uint8_t hour_word = ES;
    switch (hour) {
        case 0:
            hour_word = H_ZWOELF;
            break;
        case 1:
            hour_word = minute < 5 ? H_EIN : H_EINS;
            break;
        case 2:
            hour_word = H_ZWEI;
            break;
        case 3:
            hour_word = H_DREI;
            break;
        case 4:
            hour_word = H_VIER;
            break;
        case 5:
            hour_word = H_FUENF;
            break;
        case 6:
            hour_word = H_SECHS;
            break;
        case 7:
            hour_word = H_SIEBEN;
            break;
        case 8:
            hour_word = H_ACHT;
            break;
        case 9:
            hour_word = H_NEUN;
            break;
        case 10:
            hour_word = H_ZEHN;
            break;
        case 11:
            hour_word = H_ELF;
            break;
    }
    words.push_back(hour_word);

    switch ((minute / 5) * 5) {
        case 0:
            words.push_back(UHR);
            break;
        case 5:
            words.push_back(M_FUENF);
            words.push_back(NACH);
            break;
        case 10:
            words.push_back(M_ZEHN);
            words.push_back(NACH);
            break;
        case 15:
            words.push_back(M_VIERTEL);
            words.push_back(NACH);
            break;
        case 20:
            words.push_back(M_ZWANZIG);
            words.push_back(NACH);
            break;
        case 25:
            words.push_back(M_FUENF);
            words.push_back(VOR);
            words.push_back(HALB);
            break;
        case 30:
            words.push_back(HALB);
            break;
        case 35:
            words.push_back(M_FUENF);
            words.push_back(NACH);
            words.push_back(HALB);
            break;
        case 40:
            words.push_back(M_ZWANZIG);
            words.push_back(VOR);
            break;
        case 45:
            words.push_back(M_VIERTEL);
            words.push_back(VOR);
            break;
        case 50:
            words.push_back(M_ZEHN);
            words.push_back(VOR);
            break;
        case 55:
            words.push_back(M_FUENF);
            words.push_back(VOR);
            break;
    }

    return words;
}
/**
 * Loading animation
 */
void app_loading() {
    currentPattern();

    float pX = (NUM_COLS - 1) / 2.0 * (1 + sin(millis() / 200.0));
    float pY = (NUM_ROWS - 1) / 2.0 * (1 + cos(millis() / 250.0));

    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint8_t x = getPosX(i);
        uint8_t y = getPosY(i);
        float dSq = pow(x - pX, 2) + pow(y - pY, 2);
        if (dSq > 4) {
            leds[i] = CRGB::Black;
        } else {
            leds[i].fadeToBlackBy(dSq * 255 / 4);
        }
    }
}

/**
 * Show current time
 */

void app_time() {
    if (!hasTime) {
        app_loading();
        return;
    }

    pattern_solidColor();
    // if (currentApp == app_time) {
    //     currentPattern();
    // } else {
    //     pattern_solidColor();
    // }

    uint8_t hour = currentTime.Hour();
    uint8_t minute = currentTime.Minute();
    uint8_t second = currentTime.Second();

    uint16_t millisecondsIntoMinute = (uint16_t)second * 1000 + currentTimeMilliseconds;
    float fadePercent = (millisecondsIntoMinute < configuration.fadeDuration) ? ((float)millisecondsIntoMinute / configuration.fadeDuration) : 1;

    if (fadePercent < 1) {
        uint8_t previousHour = minute > 0 ? hour : (hour > 0 ? hour - 1 : 23);
        uint8_t previousMinute = minute > 0 ? minute - 1 : 59;
        showWords(getWordsForTime(previousHour, previousMinute), getWordsForTime(hour, minute), fadePercent);
    } else {
        showWords(getWordsForTime(hour, minute));
    }

    /* Show corner LEDs */
    uint8_t corners = minute % 5;
    if (corners < 1) {
        nscale8_video(leds + NUM_MATRIX_LEDS, 4, dim8_video(255 * (1 - fadePercent)));
    } /* Turn off all */
    else {
        uint8_t nscale = dim8_video(255 * fadePercent);
        if (corners < 2) {
            leds[NUM_MATRIX_LEDS + 3].nscale8_video((corners == 1) ? nscale : 0);
        }
        if (corners < 3) {
            leds[NUM_MATRIX_LEDS + 2].nscale8_video((corners == 2) ? nscale : 0);
        }
        if (corners < 4) {
            leds[NUM_MATRIX_LEDS + 0].nscale8_video((corners == 3) ? nscale : 0);
        }
        if (corners < 5) {
            leds[NUM_MATRIX_LEDS + 1].nscale8_video((corners == 4) ? nscale : 0);
        }
    }
}

/**
 * Solid -> just show current pattern
 */
void app_solid() {
    currentPattern();  // Don't mask out anything
}
