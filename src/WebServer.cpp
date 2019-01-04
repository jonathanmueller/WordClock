#include "WebServer.h"
#include <ESP8266WiFi.h>
 #include <FS.h>
#include <map>
#include "LEDs.h"
#include "WebCommandHandler.h"
#include "UpdateHandler.h"
#include "Configuration.h"

AsyncWebServer server(80);
bool shouldFactoryReset = false;

void onRequest(AsyncWebServerRequest *request);


void setupWebServer() {
	Serial.println(F("Setting up web server..."));
	setupWebCommandHandler();

	//First request will return 0 results unless you start scan from somewhere else (loop/setup)
	//Do not request more often than 3-5 seconds
	server.on("/scan", HTTP_GET, [](AsyncWebServerRequest * request) {
		String json = "[";
		int n = WiFi.scanComplete();
		if (n == -2) {
			WiFi.scanNetworks(true);
		} else if (n) {
			for (int i = 0; i < n; ++i) {
				if (i) json += ",";
				json += "{";
				json += "\"rssi\":" + String(WiFi.RSSI(i));
				json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
				json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
				json += ",\"channel\":" + String(WiFi.channel(i));
				json += ",\"secure\":" + String(WiFi.encryptionType(i));
				json += ",\"hidden\":" + String(WiFi.isHidden(i) ? "true" : "false");
				json += "}";
			}
			WiFi.scanDelete();
			if (WiFi.scanComplete() == -2) {
				WiFi.scanNetworks(true);
			}
		}
		json += "]";
		request->send(200, "text/json", json);
		json = String();
	});

	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
			String json = "{\"heap\":"+String(ESP.getFreeHeap())+"}";
			request->send(200, "text/json", json);
	});

	server.on("/reset", HTTP_POST, [](AsyncWebServerRequest * request) {
		bool success = false;
		if (request->hasParam("password", true)) {
  			AsyncWebParameter* p = request->getParam("password", true);
			if (p->value() == DEFAULT_SOFTAP_PASSWORD) { success = true; shouldFactoryReset = true; }
		}

		request->send(200, "text/json", success ? F("{\"success\":true}") : F("{\"success\":false}"));
	});

	server.onNotFound(onRequest);
	server.begin();
}


void updateWebServer() {
	if (shouldFactoryReset) {
		FastLED.delay(100);
		factoryReset();
	}
}

std::map<String, String> contentTypes = {
		{"html", "text/html"},
		{"htm", "text/html"},
		{"css", "text/css"},
		{"js", "text/javascript"},
		{"ttf", "application/x-font-ttf"},
		{"json", "application/json"},
		{"png", "image/png"},
		{"jpg", "image/jpeg"},
		{"jpeg", "image/jpeg"}
};

String getContentType(String path) {
	int index = path.lastIndexOf('.');
	if (index != -1) {
		String extension = path.substring(index + 1);
		extension.toLowerCase();
		if (contentTypes.find(extension) != contentTypes.end()) {
			return contentTypes[extension];
		}
	}

	return "text/plain";
}

void onRequest(AsyncWebServerRequest *request) {
	String path = request->url();
	if (path.endsWith("/")) {
		path += "index.html";
	}

	SPIFFS.begin();
	if (SPIFFS.exists(path)) {
		AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, getContentType(path));
		response->addHeader("Content-Encoding", "gzip");
		response->addHeader("Cache-Control", "max-age=3600,public");
		request->send(response);
		return;
	}

	request->send(404, "text/plain", "File not found");

	return;
}
