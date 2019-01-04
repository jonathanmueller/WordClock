
#include "LEDs.h"
#include "Configuration.h"
#include "UpdateHandler.h"
#include "WebCommandHandler.h"
#include "SerialHandler.h"
#include <ESP8266WiFi.h>
#include <Arduino.h>


void showSerialInfo() {
    Serial.println("=============================\n"
                   "Serial Commands:\n"
                   "h\tShow current heap\n"
                   "w\tShow wifi status\n"
                   "i\tShow hardware info\n"
                   "c\tConnect to SSID\n"
                   "d\tDump configuration\n"
                   "r\tReboot\n"
                   "e\tErase configuration\n"
                   "f\tFactory reset (erase config + wifi)\n"
                   "?\tShow this info\n"
                   "=============================\n");
}

String readLine() {
    String result = "";
    char c;
    while (1) {
        while (!Serial.available()) { yield(); }
        c = Serial.read();
        if (c == '\r') {
            break;
        } else {
            if (c == 0x0C || c == 0x08 || c == 0x7F) { // Backspace
                if (result.length() > 0) {
                    result.remove(result.length() - 1, 1);
                    Serial.write(c);
                }
            } else {
                result += c;
                Serial.write(c);
            }
        }
    }
    return result;
}

void setupSerialHandler() {
    showSerialInfo();
}

void dumpConfiguration() {
    for (auto it = configOptions.begin(); it != configOptions.end(); it++) {
        Serial.printf("%s: ", it->first.c_str());

        char* configdata = (char*)it->second.dataPointer;
        for (uint16_t j = 0; j < it->second.dataLength; j++) { Serial.printf("%02X ", configdata[j]); }

        Serial.printf("\n");
    }
}

void updateSerialHandler() {
    while (Serial.available() > 0) {
        byte b = Serial.read();
        switch (b) {
            case 'h':case 'H':
                Serial.printf("Current Heap: %d bytes\n", ESP.getFreeHeap());
                break;
            case 'i':case 'I':
                Serial.printf("VCC:               %d\n"
                              "Chip Id:           %d\n"
                              "Core Version:      %s\n"
                              "SDK Version:       %s\n"
                              "Reset info:        %s\n"
                              "Sketch size:       %d bytes\n"
                              "Free sketch space: %d bytes\n", ESP.getVcc(), ESP.getChipId(), ESP.getCoreVersion().c_str(), ESP.getSdkVersion(), ESP.getResetInfo().c_str(), ESP.getSketchSize(), ESP.getFreeSketchSpace());
                break;
            case 'w':case'W':
                {
                String status;
                switch (WiFi.status()) {
                    case WL_NO_SHIELD      : status = "NO_SHIELD"; break;
                    case WL_IDLE_STATUS    : status = "IDLE_STATUS"; break;
                    case WL_NO_SSID_AVAIL  : status = "NO_SSID_AVAIL"; break;
                    case WL_SCAN_COMPLETED : status = "SCAN_COMPLETED"; break;
                    case WL_CONNECTED      : status = "CONNECTED"; break;
                    case WL_CONNECT_FAILED : status = "CONNECT_FAILED"; break;
                    case WL_CONNECTION_LOST: status = "CONNECTION_LOST"; break;
                    case WL_DISCONNECTED   : status = "DISCONNECTED"; break;
                    default: status = "UNKNOWN"; break;
                }
                Serial.printf("WiFi mode: [AP=%d,STA=%d]\tWiFi status: %s, Connected clients: %d\n", (WiFi.getMode() & WIFI_AP) > 0, (WiFi.getMode() & WIFI_STA) > 0, status.c_str(), WiFi.softAPgetStationNum());
                }
                WiFi.printDiag(Serial);
                break;
            case '?': showSerialInfo(); break;
            case 'd': case'D': dumpConfiguration(); break;
            case 'c':case 'C':
                {
                    Serial.print("Enter SSID to connect (empty to cancel): ");
                    String ssid = readLine();
                    Serial.println();
                    if (ssid == "") { break; }
                    
                    Serial.print("Enter password: ");
                    String password = readLine();
                    Serial.println();

                    Serial.println("Connecting...");
                    WiFi.mode(WIFI_STA);
                    if (password == "") {
                        WiFi.begin(ssid.c_str());
                    } else {
                        WiFi.begin(ssid.c_str(), password.c_str());
                    }
                }
                break;
            case 'r':case 'R': shouldReboot = true; break;
            case 'e':case 'E':
                Serial.println("Erasing configuration...");
                delay(100);
                eraseConfigAndRestart();
                break;
            case 'f':case 'F':
                Serial.println("Doing factory reset...");
                delay(100);
                factoryReset();
                break;

        }
    }
}