/*
 * Configuration.cpp
 *
 *  Created on: 13.09.2017
 *      Author: Johnny
 */

#include "LEDs.h"
#include "Configuration.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#define CONFIG_INIT(type, name, defaultValue) name: defaultValue,
#define CONFIG_STRINGS_INIT(name, maxLength, defaultValue) defaultValue,

Configuration configuration = {
	CONFIG_TABLE(CONFIG_INIT)
    CONFIG_STRINGS(CONFIG_STRINGS_INIT)
};

#define CONFIG_MAP(type, name, defaultValue) { #name, { #type, sizeof(type), &configuration.name } },
#define CONFIG_STRINGS_MAP(name, maxLength, defaultValue) { #name, { "char*", maxLength, &configuration.name } },
CaseInsensitiveMap<ConfigOption> configOptions = {
	CONFIG_TABLE(CONFIG_MAP)
    CONFIG_STRINGS(CONFIG_STRINGS_MAP)
};

const char magicBytes[2] = {'C', 'F'};
boolean skipWritingConfiguration = false;

#define MAGIC_SIZE ((sizeof(magicBytes)/sizeof(magicBytes[0])))

#define CONFIG_OFFSET MAGIC_SIZE
#define CONFIG_SIZE sizeof(Configuration)

#define EEPROM_SIZE (CONFIG_OFFSET + CONFIG_SIZE)

void setupConfiguration() {
    Serial.println(F("Setting up configuration..."));
	EEPROM.begin(EEPROM_SIZE);

	char magic[MAGIC_SIZE];
    EEPROM.get(0, magic);

    if (memcmp(magic, magicBytes, MAGIC_SIZE) == 0) {
        EEPROM.get(CONFIG_OFFSET, configuration);
    } else {
        Serial.println(F("Loading default configuration..."));

        EEPROM.put(0, magicBytes);
        writeConfiguration(); // Writing default config
    }
}

void writeConfiguration() {
    if (skipWritingConfiguration) { return; }
    EEPROM.put(CONFIG_OFFSET, configuration);
    EEPROM.commit();
}

void eraseConfigAndRestart() {
    // Reset configuration by invalidating magic bytes
    EEPROM.put(0, "XX");
    EEPROM.commit();

    ESP.reset();
}

void factoryReset() {
    ESP.eraseConfig();
    WiFi.disconnect();
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SOFTAP_NAME, DEFAULT_SOFTAP_PASSWORD);
		

    eraseConfigAndRestart();
}