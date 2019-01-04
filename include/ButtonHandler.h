#pragma once

#define BUTTON_PIN 0                // PIN for softAP button
#define RESET_BUTTON_DURATION 50    // In tenths of seconds (eg. 50 => 5s)

void setupButtonHandler();
void updateButtonHandler();