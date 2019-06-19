#include <vector>

#include "TimeKeeper.h"

#define FASTLED_INTERNAL
#include <FastLED.h>
#include <ESP8266Wifi.h>
#include <Wire.h>
#include <RtcDS1307.h>
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#include "Configuration.h"

RtcDS1307<TwoWire> rtc(Wire);

const char* timeServerHostName = "api.timezonedb.com";
const char* timeServerRequest = "GET /v2/get-time-zone?key=5KJACIM0E81F&by=zone&zone=Europe/Berlin&format=json&fields=timestamp HTTP/1.1\r\nHost: api.timezonedb.com\r\nConnection: close\r\n\r\n";

struct HTTPResponse {
	int code;
	int length;
	String body;
};

RtcDateTime currentTime;
uint16_t currentTimeMilliseconds(0);
unsigned long int millisAtLastFullSecond(0);
bool hasTime = false;
RtcDateTime lastSyncedTime;

AsyncClient tcpClient;
unsigned long int requestStartTime;
std::vector<char> responseBuffer;

void connectedToTimeServer(void* _, AsyncClient *tcpClient);
void dataFromTimeServer(void* _, AsyncClient *tcpClient, void *data, size_t len);
void disconnectedFromTimeServer(void* _, AsyncClient *tcpClient);
HTTPResponse parseHTTPResponse(String responseString);
void syncTimeWithRTC();

void setupTimeKeeper() {
	Serial.println(F("Setting up time keeper..."));
	rtc.Begin();
	rtc.SetIsRunning(true);

	syncTimeWithRTC();

	tcpClient.onConnect(connectedToTimeServer);
	tcpClient.onData(dataFromTimeServer);
	tcpClient.onDisconnect(disconnectedFromTimeServer);
}

void updateTimeKeeper() {
	EVERY_N_MINUTES_I(syncWithWeb, configuration.synchronizationInterval) {
		syncTimeWithWeb();
	}
	syncWithWeb.setPeriod(configuration.synchronizationInterval);


	EVERY_N_SECONDS(60) { syncTimeWithRTC(); }
	EVERY_N_SECONDS(1) 	{ currentTime += 1;	millisAtLastFullSecond = millis(); }
	currentTimeMilliseconds = millis() - millisAtLastFullSecond;
}

String timeToISOString(RtcDateTime& dt) {
	char result[25];
	sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", dt.Year(), dt.Month(), dt.Day(), dt.Hour(), dt.Minute(), dt.Second());
	return String(result);
}

RtcDateTime timeFromISOString(String& str) {
	uint16_t year = 0;
	uint8_t month = 0, dayOfMonth = 0, hour = 0, minute = 0, second = 0;
	if (str.length() == 24) {
		year = str.substring(0, 4).toInt();
		month = str.substring(5,7).toInt();
		dayOfMonth = str.substring(8,10).toInt();
		hour = str.substring(11,13).toInt();
		minute = str.substring(14,16).toInt();
		second = str.substring(17,19).toInt();
	}
	return RtcDateTime(year, month, dayOfMonth, hour, minute, second);
}

void syncTimeWithRTC() {
	if (rtc.IsDateTimeValid()) {
		setCurrentTime(rtc.GetDateTime());
	}
}

void setCurrentTime(const RtcDateTime& dt) {
	currentTime = dt;
	currentTimeMilliseconds = 0;
	millisAtLastFullSecond = millis();
	hasTime = true;
}

void connectedToTimeServer(void* _, AsyncClient *tcpClient) {
	Serial.println("Connected to time server...");
	requestStartTime = millis();
	tcpClient->write(timeServerRequest);
}

void dataFromTimeServer(void* _, AsyncClient *tcpClient, void *data, size_t len) {
	responseBuffer.insert(responseBuffer.end(), (char*) data, ((char*) data) + len);
}

void disconnectedFromTimeServer(void* _, AsyncClient *tcpClient) {
	unsigned long int delay = millis() - requestStartTime;

	// 5s is unacceptable
	if (delay > 5000) {
		Serial.println(F("Got the time, but the request took too long, ignoring."));

		if (!hasTime) {
			// Retry if the time is not yet there
			syncTimeWithWeb();
		}
		return;
	}
	
	responseBuffer.push_back((char)0);
	String responseString(responseBuffer.data());

	HTTPResponse response = parseHTTPResponse(responseString);
	responseBuffer.clear();

	DynamicJsonDocument root(1024);
	DeserializationError error = deserializeJson(root, response.body);
	if (error) {
		return;
	}

	// {"status":"OK","message":"","timestamp":1504739460}
	if (root["status"] == "OK") {
		long time = root["timestamp"];
		RtcDateTime datetime; datetime.InitWithEpoch64Time(time);
		rtc.SetDateTime(datetime);
		lastSyncedTime = RtcDateTime(datetime);

		setCurrentTime(datetime);
		
		Serial.println(F("Time updated."));
	} else {
		Serial.println(F("Invalid response."));
	}
}

void syncTimeWithWeb() {
	if (tcpClient.connected()) {
		tcpClient.close(true);
	}

	responseBuffer.clear();
	tcpClient.connect(timeServerHostName, 80);
}

HTTPResponse parseHTTPResponse(String responseString) {
	HTTPResponse response;
	response.code = 0;
	response.length = 0;
	response.body = "";

	bool chunkedResponse = false;

	String headerLine;
	int offset = 0;

	do {
		int index = responseString.indexOf('\n', offset);
		headerLine = responseString.substring(offset, index);
		headerLine.trim(); // remove \r
		offset = index + 1;

		if (headerLine.startsWith("HTTP/1.")) {
			response.code = headerLine.substring(9, headerLine.indexOf(' ', 9)).toInt();
		} else if (headerLine.indexOf(':')) {
			String headerName = headerLine.substring(0, headerLine.indexOf(':'));
			String headerValue = headerLine.substring(headerLine.indexOf(':') + 1);
			headerValue.trim();

			if (headerName.equalsIgnoreCase("Content-Length")) {
				response.length = headerValue.toInt();
			}

			if (headerName.equalsIgnoreCase("Transfer-Encoding")) {
				if (headerValue.equalsIgnoreCase("chunked")) {
					chunkedResponse = true;
				}
			}
		}
	} while (headerLine != "");

	if (!chunkedResponse) {
		response.body = responseString.substring(offset);
	} else {
		response.body = "";
		response.length = 0;
		String chunkHeader;
		int len = 0;
		do {
			int index = responseString.indexOf('\n', offset);
			chunkHeader = responseString.substring(offset, index);
			chunkHeader.trim(); // remove \r
			offset = index + 1;

			// Convert length from base 16
			len = (uint32_t) strtol((const char *) chunkHeader.c_str(), NULL, 16);
			response.length += len;

			// data left?
			if (len > 0) {
				response.body += responseString.substring(offset, offset + len);
			}

			offset += len + 2;
		} while (len > 0);
	}

	return response;
}
