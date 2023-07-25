#pragma once

#include <vector>
#include <set>

#define FASTLED_INTERNAL
#undef FASTLED_ALLOW_INTERRUPTS
#define FASTLED_ALLOW_INTERRUPTS 0
#undef delay
#include <FastLED.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DATA_PIN			13
#define LED_TYPE			WS2811
#define COLOR_ORDER 		GRB
#define FRAMES_PER_SECOND  	120

#define NUM_ROWS			10
#define NUM_COLS			11
#define NUM_LEDS			(NUM_ROWS*NUM_COLS + 4)

#define NUM_MATRIX_LEDS		(NUM_ROWS*NUM_COLS)

#define STATUS_LED_BLINK_DELAY_FAST 100	// Status LED fast blink duration
#define STATUS_LED_BLINK_DELAY_SLOW 500	// Status LED slow blink duration

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


/**
 * Use this to set the LEDs
 */
extern CRGB *leds;               // [NUM_LEDS];
extern CRGB* const leds_main;    // [NUM_LEDS];
extern CRGB* const leds_buffer;  // [NUM_LEDS];

/**
 * User these to get the position of a specific LED_TYPE
 */
uint8_t getPosX(uint8_t led);
uint8_t getPosY(uint8_t led);

/**
 * Rotating "base color" that can be used by the patterns
 */
extern uint8_t gHue;
extern float gHueFraction;

/**
 * Status LED functions
 */
typedef enum StatusLEDState {
	Off, FlashingFast, FlashingSlow, On
} StatusLEDState;

#define INFINITE -1

/* Words ENUM */
enum { ES, IST, M_FUENF, M_ZEHN, M_ZWANZIG, M_DREI, M_VIERTEL, NACH, VOR, HALB, H_ZWOELF, H_ZWEI, H_EIN, H_EINS, H_SIEBEN, H_DREI, H_FUENF, H_ELF, H_NEUN, H_VIER, H_ACHT, H_ZEHN, H_SECHS, UHR };

/* Glyphs ENUM */
typedef enum { Checkmark, Cross/*, Error*/ } Glyph;

void showWords(const std::vector<uint8_t> &wordArray, const std::vector<uint8_t> &wordArray2 = {}, float fadePercent = 0);
void showGlyph(Glyph glyph);

// Setup/Loop
void setupLEDs();
void updateLEDs();
