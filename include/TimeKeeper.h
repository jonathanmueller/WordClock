#pragma once

#include <RtcDS1307.h>
#include <Wire.h>

extern RtcDS1307<TwoWire> rtc;
extern RtcDateTime currentTime;
extern uint16_t currentTimeMilliseconds;
extern bool hasTime;
extern RtcDateTime lastSyncedTime;

void setupTimeKeeper();
void updateTimeKeeper();

void syncTimeWithWeb();
void setCurrentTime(const RtcDateTime& time);
String timeToISOString(RtcDateTime& dt);
RtcDateTime timeFromISOString(String& str);