#include "WebCommandHandler.h"

#include <vector>

#include <ArduinoJson.h>

#include "Configuration.h"
#include "WebServer.h"
#include "LEDs.h"
#include "Apps.h"
#include "Patterns.h"
#include "TimeKeeper.h"
#include "TemperatureKeeper.h"
#include "UpdateHandler.h"

#include <set>
#include <map>

extern "C" {
#include "user_interface.h"
}

#define RESPOND(client, message) if (client != nullptr) { client->text(message); }

AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

std::set<uint32_t> authenticatedClients;
extern std::map<String, void (*)(AsyncWebSocketClient*, std::vector<String>&)> commandHandlers;

const String successStatus = "{\"success\":true}";
const String expectedParameter = "{\"success\": false,\"error\":\"Expected Parameter\"}";
const String unknownCommand = "{\"success\": false,\"error\":\"Unkown Command\"}";

String joinArgs(const std::vector<String>& args, size_t startIndex) {
	if (startIndex < args.size()) {
		String joinedArgs = args[startIndex++];
		while (startIndex < args.size()) {
			joinedArgs += " " + args[startIndex];
			startIndex++;
		}
		return joinedArgs;
	} else {
		return "";
	}
}

void wifi(AsyncWebSocketClient *client, std::vector<String>& args) {
	if (args.size() == 0) {
		RESPOND(client, F("Subcommands: status, connect, softAP, disconnect"));
	} else {
		args[0].toLowerCase();
		if (args[0] == F("status")) {
			DynamicJsonDocument root(1024);
			root["success"] = true;
			root["mode"] = (WiFi.getMode() & WIFI_AP) ? "softAP" : "station";
			root["ssid"] = WiFi.SSID();
			root["connected"] = WiFi.isConnected();

			softap_config config;
			wifi_softap_get_config(&config);
			root["softAPName"] = (char*) config.ssid;

			String response;
			serializeJson(root, response);
			RESPOND(client, response);
		} else if (args[0] == F("connect")) {
			RESPOND(client, successStatus);

			WiFi.mode(WIFI_STA);

			if (args.size() == 1) {
				WiFi.begin();
			} else if (args.size() == 2) {
				WiFi.begin(args[1].c_str());
			} else {
				WiFi.begin(args[1].c_str(), args[2].c_str());
			}
		} else if (args[0] == F("softap")) {
			RESPOND(client, successStatus);

			WiFi.mode(WIFI_AP);

			if (args.size() == 2) {
				WiFi.softAP(args[1].c_str());
			} else if (args.size() > 2) {
				WiFi.softAP(args[1].c_str(), args[2].c_str());
			}
		} else if (args[0] == F("disconnect")) {
			WiFi.enableSTA(false);
			RESPOND(client, successStatus);
		} else {
			RESPOND(client, unknownCommand);
		}
	}
}

#define GET_DATA(dest, addr) dest = *addr;
#define GET_STRING(dest, addr) dest = String((char*)addr);

#define ASSIGN_INT(addr, dataLength, str) *addr = str.toInt();
#define ASSIGN_FLOAT(addr, dataLength, str) *addr = str.toFloat();
#define ASSIGN_C_STRING(addr, dataLength, str) strncpy((char*)addr, str.c_str(), dataLength); ((char*)addr)[dataLength] = 0;

#define TYPE_LIST(FIRST, SECOND) \
	FIRST(bool, ASSIGN_INT, GET_DATA) \
	SECOND(uint8_t, ASSIGN_INT, GET_DATA) \
	SECOND(uint32_t, ASSIGN_INT, GET_DATA) \
	SECOND(uint16_t, ASSIGN_INT, GET_DATA) \
	SECOND(uint64_t, ASSIGN_INT, GET_DATA) \
	SECOND(char, ASSIGN_INT, GET_DATA) \
	SECOND(int8_t, ASSIGN_INT, GET_DATA) \
	SECOND(int16_t, ASSIGN_INT, GET_DATA) \
	SECOND(int64_t, ASSIGN_INT, GET_DATA) \
	SECOND(int32_t, ASSIGN_INT, GET_DATA) \
	SECOND(long, ASSIGN_INT, GET_DATA) \
	SECOND(int, ASSIGN_INT, GET_DATA) \
	SECOND(short, ASSIGN_INT, GET_DATA) \
	SECOND(char*, ASSIGN_C_STRING, GET_STRING)
	//SECOND(float, ASSIGN_FLOAT)

void configToJSON(JsonObject &settings, String key) {
	if (configOptions.find(key) != configOptions.end()) {
		ConfigOption config = configOptions[key];

		#define IF_CAST(type, _, ASSIGN) if (config.typeName == F(#type)) { ASSIGN(settings[key], ((type*)config.dataPointer)) }
		#define ELSE_IF_CAST(type, _, ASSIGN) else IF_CAST(type, _, ASSIGN)

		TYPE_LIST(IF_CAST, ELSE_IF_CAST)
	}
}

void writeToConfig(String key, String value) {
	if (configOptions.find(key) != configOptions.end()) {
		ConfigOption config = configOptions[key];

		#define IF_ASSIGN(type, ASSIGN, _) if (config.typeName == F(#type)) { ASSIGN(((type*)config.dataPointer), config.dataLength, value) }
		#define ELSE_IF_ASSIGN(type, ASSIGN, _) else IF_ASSIGN(type, ASSIGN, _)

		TYPE_LIST(IF_ASSIGN, ELSE_IF_ASSIGN)
	}
}

void settings(AsyncWebSocketClient *client, std::vector<String> &args) {
	if (args.size() == 0) {
		RESPOND(client, F("Subcommands: get, set"));
	} else {
		args[0].toLowerCase();
		if (args[0] == "get") {
			DynamicJsonDocument root(1024);
			root["success"] = true;
			JsonObject settings = root.createNestedObject("settings");
			if (args.size() == 1) {
				// Get all parameters
				for (auto it = configOptions.begin(); it != configOptions.end(); it++) {
					if (it->first == "password") { continue; }
					configToJSON(settings, it->first);
				}
			} else if (args[1] != "password") {
				configToJSON(settings, args[1]);
			}

			String response;
			serializeJson(root, response);
			RESPOND(client, response);
		} else if (args[0] == "set") {
			writeToConfig(args[1], joinArgs(args, 2));
			writeConfiguration();
			RESPOND(client, successStatus);
		} else {
			RESPOND(client, unknownCommand);
		}
	}
}

void display(AsyncWebSocketClient *client, std::vector<String>& args) {
	if (args.size() == 0) {
		RESPOND(client, F("Subcommands: colorCorrection, color, show, pattern"));
		return;
	} else {
		args[0].toLowerCase();
		if (args[0] == F("colorcorrection")) {
			if (args.size() == 4) {
				int r = atoi(args[1].c_str());
				int g = atoi(args[2].c_str());
				int b = atoi(args[3].c_str());

				FastLED.setCorrection(CRGB(r, g, b));
			} else if (args.size() == 2) {
				args[1].toLowerCase();
				if (args[1] == F("ledstrip")) {
					FastLED.setCorrection(TypicalLEDStrip);
				} else if (args[1] == F("8mmpixel")) {
					FastLED.setCorrection(Typical8mmPixel);
				} else if (args[1] == F("pixelstring")) {
					FastLED.setCorrection(TypicalPixelString);
				} else if (args[1] == F("smd5050")) {
					FastLED.setCorrection(TypicalSMD5050);
				} else if (args[1] == F("none")) {
					FastLED.setCorrection(UncorrectedColor);
				} else {
					RESPOND(client, unknownCommand);
					return;
				}
			} else {
				RESPOND(client, expectedParameter);
				return;
			}

			RESPOND(client, successStatus);
		} else if (args[0] == "show") {
			if (args.size() == 1) {
				RESPOND(client, F("Subcommands: cycle, time, solid"));
			} else {
				args[1].toLowerCase();

				if (args[1] == "cycle") {
					currentApp = app_cycle;
				} else if (args[1] == "time") {
					currentApp = app_time;
				} else if (args[1] == "solid") {
					currentApp = app_solid;
				} else {
					RESPOND(client, unknownCommand);
					return;
				}

				writeToConfig("app", args[1]);
				writeConfiguration();

				RESPOND(client, successStatus);
			}
		} else if (args[0] == "color") {
			if (args.size() == 4) {
				int r = atoi(args[1].c_str());
				int g = atoi(args[2].c_str());
				int b = atoi(args[3].c_str());

				solidLEDColor = CRGB(r, g, b);
				
				writeToConfig("color", String(r) + " " + String(g) + " " + String(b));
				writeConfiguration();
				RESPOND(client, successStatus);
			} else {
				RESPOND(client, expectedParameter);
				return;
			}
		} else if (args[0] == "pattern") {
			if (args.size() == 1) {
				RESPOND(client, F("Subcommands: cycle, rainbow, rainbowWithGlitter, noise, waves, confetti, sinelon, juggle, flashing, bpm, solid"));
			} else {
				args[1].toLowerCase();
				if (args[1] == "cycle") {
					currentPattern = pattern_cycle;
				} else if (args[1] == "rainbow") {
					currentPattern = pattern_rainbow;
				} else if (args[1] == "rainbowwithglitter") {
					currentPattern = pattern_rainbowWithGlitter;
				} else if (args[1] == "noise") {
					currentPattern = pattern_noise;
				} else if (args[1] == "waves") {
					currentPattern = pattern_waves;
				} else if (args[1] == "confetti") {
					currentPattern = pattern_confetti;
				} else if (args[1] == "sinelon") {
					currentPattern = pattern_sinelon;
				} else if (args[1] == "juggle") {
					currentPattern = pattern_juggle;
				} else if (args[1] == "flashing") {
					if (args.size() == 6) {
						flashingConfig.onTime = atoi(args[2].c_str());
						flashingConfig.offTime = atoi(args[3].c_str());
						flashingConfig.repetitions = atoi(args[4].c_str());
						flashingConfig.delayTime = atoi(args[5].c_str());
					} else {
						RESPOND(client, F("Usage: {ontime} {offtime} {repetitions} {delay}"));
						return;
					}

					currentPattern = pattern_flashing;
				} else if (args[1] == "bpm") {
					currentPattern = pattern_bpm;
				} else if (args[1] == "solid") {
					currentPattern = pattern_solidColor;
				} else {
					RESPOND(client, unknownCommand);
					return;
				}
				
				writeToConfig("pattern", args[1]);
				writeConfiguration();
				RESPOND(client, successStatus);
			}
		} else {
			RESPOND(client, unknownCommand);
		}
	}

}

void time(AsyncWebSocketClient *client, std::vector<String>& args) {
	if (args.size() == 0) {
		RESPOND(client, F("Subcommands: reload, get, set"));
		return;
	} else {
		args[0].toLowerCase();
		if (args[0] == F("reload")) {
			syncTimeWithWeb();
			RESPOND(client, successStatus);
			return;
		} else if (args[0] == F("get")) {
			DynamicJsonDocument root(1024);
			root["success"] = true;
			root["time"] = timeToISOString(currentTime);
			root["lastsynced"] = timeToISOString(lastSyncedTime);

			String response;
			serializeJson(root, response);
			RESPOND(client, response);
		} else if (args[0] == F("set")) {
			if (args.size() == 2 && args[1].length() == 24) {
				RtcDateTime dt = timeFromISOString(args[1]);
				
				DynamicJsonDocument root(1024);

				if (!dt.Year()) {
					root["success"] = false;
				} else {
					setCurrentTime(dt);
					root["success"] = true;
					root["time"] = timeToISOString(currentTime);
				}

				String response;
				serializeJson(root, response);
				RESPOND(client, response);
			} else {
				RESPOND(client, F("Usage: {datetime-ISO-8601} (e.g. 2018-05-11T15:43:31.000Z)"));
				return;
			}
		} else {
			RESPOND(client, unknownCommand);
			return;
		}
	}
}

// void temperature(AsyncWebSocketClient *client, std::vector<String>& args) {
// 	DynamicJsonBuffer jsonBuffer;
// 	JsonObject& root = jsonBuffer.createObject();
// 	if (temperatureSensorFound) {
// 		root["success"] = true;
// 		root["temperature"] = currentTemperature;
// 	} else {
// 		root["success"] = false;
// 		root["error"] = "Temperature sensor not found";
// 	}

// 	String response;
// 	root.printTo(response);
// 	RESPOND(client, response);
// 	return;
// }

void doAction(AsyncWebSocketClient *client, std::vector<String>& args) {
	if (args.size() == 0) {
		RESPOND(client, F("Subcommands: reboot, clearConfig, factoryReset"));
		return;
	} else {
		args[0].toLowerCase();
		if (args[0] == F("reboot")) {
			RESPOND(client, successStatus);
			shouldReboot = true;
			return;
		} else if (args[0] == F("clearconfig")) {
			RESPOND(client, successStatus);
			delay(100);
			eraseConfigAndRestart();
			return;
		} else if (args[0] == F("factoryreset")) {
			RESPOND(client, successStatus);
			delay(100);
			factoryReset();
			return;
		} else {
			RESPOND(client, unknownCommand);
			return;
		}
	}
}

void help(AsyncWebSocketClient *client, std::vector<String>& args) {
	String commands = F("The following commands are available:\n");
	for (std::map<String, void (*)(AsyncWebSocketClient*, std::vector<String>&)>::iterator it = commandHandlers.begin(); it != commandHandlers.end(); ++it) {
		if (it != commandHandlers.begin()) {
			commands += ", ";
		}
		commands += it->first;
	}
	RESPOND(client, commands);
}

// void app(AsyncWebSocketClient *client, std::vector<String>& args) {
// 	if (args.size() == 0) {
// 		RESPOND(client, F("Subcommands: -"));
// 		return;
// 	} else {
//		args[0].toLowerCase();
// 		if (args[0] == "scoreboard") {
// 			currentApp = app_scoreboard;
// 			RESPOND(client, successStatus);
// 		} else if (args[0] == "countdown") {
// 			currentApp = app_countdown;
// 			RESPOND(client, successStatus);
// 		} else if (args[0] == "stopwatch") {
// 			app_stopwatch_start_time = millis();
// 			app_stopwatch_running = true;
// 			currentApp = app_stopwatch;
// 			RESPOND(client, successStatus);
// 		} else {
// 			RESPOND(client, unknownCommand);
// 		}
// 	}
// }


std::map<String, void (*)(AsyncWebSocketClient*, std::vector<String>&)> commandHandlers = {
		{ "display", display },
		{ "wifi", wifi },
		{ "settings", settings },
		{ "time", time },
		// { "temperature", temperature },
		// { "app", app },
		{ "do", doAction },
		{ "help", help }
};

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

void setupWebCommandHandler() {
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}

inline boolean isAuthenticated(uint32_t clientId) {
	return authenticatedClients.find(clientId) != authenticatedClients.end();
}

void handleCommand(AsyncWebSocketClient *client, const String &command, std::vector<String>& args) {
	if (commandHandlers.find(command) != commandHandlers.end()) {
		commandHandlers[command](client, args);
	} else {
		RESPOND(client, unknownCommand);
	}
}

void handleMessage(AsyncWebSocketClient *client, const String &message) {
	std::vector<String> args;

	int index = message.indexOf(' ');
	if (index == -1) {
		handleCommand(client, message, args);
	} else {
		String command = message.substring(0, index);
		command.toLowerCase();

		unsigned int lastIndex = index + 1;

		while ((index = message.indexOf(' ', lastIndex)) != -1) {
			args.push_back(message.substring(lastIndex, index));
			lastIndex = index + 1;
		}
		if (lastIndex < message.length()) {
			args.push_back(message.substring(lastIndex));
		}

		handleCommand(client, command, args);
	}
}

void executeCommand(const String &command) {
	boolean savedSkip = skipWritingConfiguration;
	skipWritingConfiguration = true;
	
	handleMessage(nullptr, command);

	skipWritingConfiguration = savedSkip;
}

#define HEX_BUF_LEN (3*sizeof(Configuration)+1)
#define DEBUG_DUMP_CONFIG(client)
/* DEBUG: Send configuration hex data */
// void DEBUG_DUMP_CONFIG(AsyncWebSocketClient *client) {
// 	char buf[HEX_BUF_LEN]; buf[HEX_BUF_LEN-1] = 0; char* configdata = (char*)&configuration;
// 	for (uint16_t j = 0; j < sizeof(configuration); j++) { sprintf(&buf[3*j], "%02X ", configdata[j]); }
// 	RESPOND(client, String(buf));
// }

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
	if (type == WS_EVT_CONNECT) {
		//client connected
		client->printf("{\"msg\":\"Welcome. Please enter the password :)\",\"clientId\":%u}", client->id());
	} else if (type == WS_EVT_DISCONNECT) {
		//client disconnected
		authenticatedClients.erase(client->id());
	} else if (type == WS_EVT_ERROR) {
		//error was received from the other end
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*) arg), (char*) data);
	} else if (type == WS_EVT_PONG) {
		//pong message was received (in response to a ping request maybe)
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*) data : "");
	} else if (type == WS_EVT_DATA) {
		//data packet
		AwsFrameInfo * info = (AwsFrameInfo*) arg;
		if (info->final && info->index == 0 && info->len == len) {
			if (info->opcode != WS_TEXT) {
				RESPOND(client, F("{\"success\":false,\"error\":\"Plain text only\"}"));
				DEBUG_DUMP_CONFIG(client);
				return;
			}

			if (!isAuthenticated(client->id())) {
				if (len != strlen(configuration.password) || strncmp((char*) data, configuration.password, len) != 0) {
					RESPOND(client, F("{\"success\":false,\"error\":\"Wrong password\"}"));
					DEBUG_DUMP_CONFIG(client);
					return;
				}

				RESPOND(client, successStatus);				
				authenticatedClients.insert(client->id());
				DEBUG_DUMP_CONFIG(client);
				return;
			}

			handleMessage(client, String(std::string((char*) data, info->len).c_str()));						
			DEBUG_DUMP_CONFIG(client);
		} else if (info->final) {
			RESPOND(client, F("{\"success\":false,\"error\":\"Message too long\"}"));
		}
	}
}

