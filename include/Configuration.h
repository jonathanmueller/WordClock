#pragma once

#include <Arduino.h>
#include <map>
#include <typeinfo>

#define DEFAULT_SOFTAP_NAME (getDefaultSoftAPName().c_str())
#define DEFAULT_SOFTAP_PASSWORD	"ZDQHE82S6G" // http://passwordsgenerator.net/plus/, http://tinyqr.com/qr-generator.php

inline String getDefaultSoftAPName() {
	char hexId[14];
	sprintf(hexId, "LEDClock_%04X", ESP.getChipId() & 0xFFFF);
	return String(hexId);
}

#define CONFIG_TABLE(HANDLE_ENTRY) \
	HANDLE_ENTRY(uint8_t, 	brightness, 				20			)	/* Brightness in percent (0 - 100) 		*/	\
	HANDLE_ENTRY(uint16_t, 	fadeDuration,				1000		) 	/* Time fade duration					*/	\
	HANDLE_ENTRY(uint16_t, 	synchronizationInterval,	30			) 	/* Synchronization interval in minutes	*/

#define CONFIG_STRINGS(HANDLE_ENTRY) \
	HANDLE_ENTRY(pattern, 			32, 	"cycle"					)	/* The pattern							*/	\
	HANDLE_ENTRY(app, 				32, 	"cycle"					)	/* The app 								*/	\
	HANDLE_ENTRY(color, 			11, 	"0 255 255"				)	/* The solidLED color 					*/	\
	HANDLE_ENTRY(password, 			32, 	"thepassword"			)	/* The password 						*/	\
	HANDLE_ENTRY(softAPName,		32, 	""						)	/* The softAP name (empty = DEFAULT)	*/	\
	HANDLE_ENTRY(softAPPassword, 	32, 	DEFAULT_SOFTAP_PASSWORD	)	/* The softAP password 					*/

// Define a struct with the config as fields
#define CONFIG_STRUCT(type, name, defaultValue) type name;
#define CONFIG_STRINGS_STRUCT(name, maxLength, defaultValue) char name[maxLength+1];

typedef struct {
	CONFIG_TABLE(CONFIG_STRUCT)
	CONFIG_STRINGS(CONFIG_STRINGS_STRUCT)
} Configuration;

struct CaseInsensitiveComparator {
    bool operator()(const String& a, const String& b) const noexcept {
    		return ::strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
template <typename T> using CaseInsensitiveMap = std::map<String, T, CaseInsensitiveComparator>;

typedef struct {
	String typeName;
	uint8_t dataLength;
	void* dataPointer;
} ConfigOption;

// Contains the map from string to ConfigOption (e.g. "brightness" => {"uint8_t", 1, &configuration.brightness} )
extern CaseInsensitiveMap<ConfigOption> configOptions;

extern Configuration configuration;

extern boolean skipWritingConfiguration;

void setupConfiguration();
void writeConfiguration();
void eraseConfigAndRestart();
void factoryReset();
