#pragma once

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern AsyncEventSource events; // event source (Server-Sent events)

void setupWebServer();
void updateWebServer();
