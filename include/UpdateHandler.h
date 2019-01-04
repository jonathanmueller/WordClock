#pragma once

#include "Arduino.h"

extern boolean shouldReboot;

void setupUpdateHandler();
void rebootIfNecessary();
