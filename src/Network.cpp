#include "Network.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <vector>

#include "Configuration.h"
#include "Firmware.h"
#include "LEDs.h"
#include "TimeKeeper.h"

const char* hostName = "ledclock";
std::vector<WiFiEventHandler> wifiHandlers;

void setupNetwork() {
#ifndef ESPHOME
    Serial.println(F("Setting up network..."));
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin();

    MDNS.begin(hostName);
    MDNS.addService("http", "tcp", 80);
#endif

    wifiHandlers.push_back(WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
        syncTimeWithWeb();
    }));

    wifiHandlers.push_back(WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {
        // setStatusLED(FlashingFast, CRGB::Blue, 2);
    }));

    wifiHandlers.push_back(WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
        switch (event.reason) {
            case WIFI_DISCONNECT_REASON_NO_AP_FOUND:
                WiFi.setAutoReconnect(false);
                break;
            default:
                break;
        }
        // setStatusLED(FlashingSlow, CRGB::Red, 5);
    }));

    wifiHandlers.push_back(WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& event) {
        // setStatusLED(FlashingFast, CRGB::Blue, 2);
    }));

    wifiHandlers.push_back(WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected& event) {
        // setStatusLED(FlashingSlow, CRGB::Red, 5);
    }));
}

void switchNetworkMode() {
    boolean isSoftAPMode = !(WiFi.getMode() & WIFI_AP);
    Serial.println(isSoftAPMode ? F("Switching to softAP mode") : F("Switching to station mode"));
    WiFi.mode(isSoftAPMode ? WIFI_AP : WIFI_STA);
    if (!isSoftAPMode) {
        WiFi.begin();
        WiFi.setAutoReconnect(true);
    } else {
        if (configuration.softAPName[0] == 0) {
            WiFi.softAP(DEFAULT_SOFTAP_NAME, configuration.softAPPassword);
        } else {
            WiFi.softAP(configuration.softAPName, configuration.softAPPassword);
        }
    }

    // Status LED: Show SoftAP status
    // CRGB color = isSoftAPMode ? CRGB::Green : CRGB::Red;
    // setStatusLED(FlashingFast, color, 2);
}