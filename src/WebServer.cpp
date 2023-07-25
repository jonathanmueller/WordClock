#include "WebServer.h"


#include <ESP8266WiFi.h>
// #include <AsyncHTTPRequest_Generic.h>
#include <map>

#include "Configuration.h"
#include "LEDs.h"
#include "LittleFS.h"
#include "UpdateHandler.h"
#include "WebCommandHandler.h"

AsyncWebServer server(80);
bool shouldFactoryReset = false;




void onRequest(AsyncWebServerRequest *request);


// AsyncHTTPRequest asyncHttpRequest;
// void downloadFile(void *optParm, AsyncHTTPRequest *asyncHttpRequest, int readyState);
// File downloadedFile;

void setupWebServer() {
    Serial.println(F("Setting up web server..."));
    setupWebCommandHandler();

    // First request will return 0 results unless you start scan from somewhere else (loop/setup)
    // Do not request more often than 3-5 seconds
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
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

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"heap\":" + String(ESP.getFreeHeap()) + "}";
        request->send(200, "text/json", json);
    });

    server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool success = false;
        if (request->hasParam("password", true)) {
            AsyncWebParameter *p = request->getParam("password", true);
            if (p->value() == DEFAULT_SOFTAP_PASSWORD) {
                success = true;
                shouldFactoryReset = true;
            }
        }

        request->send(200, "text/json", success ? F("{\"success\":true}") : F("{\"success\":false}"));
    });
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        LittleFS.begin();
        AsyncResponseStream * response = request->beginResponseStream("application/json");
        response->print('[');
        Dir dir = LittleFS.openDir("/");

        bool first = true;
        while (dir.next()) {
            if (!first) {
                response->print(',');
            }
            first = false;

            response->print("{\"name\":\"");
            response->print(dir.fileName());
            response->print("\",size:");
            response->print(dir.fileSize());
            response->print('}');
        }

        response->print("]");
        request->send(response);
    });

    // server.on("/download", HTTP_POST, [](AsyncWebServerRequest *request) {
    //     bool authorized = false;
    //     if (request->hasParam("password", true)) {
    //         AsyncWebParameter *p = request->getParam("password", true);
    //         if (p->value() == configuration.password) {
    //             authorized = true;
    //             bool success = false;

    //             AsyncWebParameter *url = request->getParam("url", true);
    //             if (url != nullptr) {
    //                 if (asyncHttpRequest.readyState() == readyStateUnsent || asyncHttpRequest.readyState() == readyStateDone) {
    //                     LittleFS.begin();
    //                     downloadedFile = LittleFS.open("download.bin", "w");
    //                     asyncHttpRequest.onReadyStateChange(downloadFile);
    //                     if (asyncHttpRequest.open("GET", url->value().c_str())) {
    //                         asyncHttpRequest.send();
    //                         success = true;
    //                     }
    //                 }
    //             }

    //             request->send(success ? 200 : 500, "text/json", success ? F("{\"success\":true}") : F("{\"success\":false}"));
    //         }
    //     }
        
    //     if (!authorized) {
    //         request->send(401, "text/json", F("{\"success\":false,message:\"Unauthorized\"}"));
    //     }

    // });

    server.onNotFound(onRequest);
    server.begin();
}

// void downloadFile(void *optParm, AsyncHTTPRequest *asyncHttpRequest, int readyState) {
//     if (readyState == readyStateHdrsRecvd) {
//         if (asyncHttpRequest->responseHTTPcode() != 200) {
//             downloadedFile.close();
//             asyncHttpRequest->abort();
//         }
//     } else if (readyState == readyStateDone) {
//         size_t available = asyncHttpRequest->available();
//         Serial.printf("Downloaded %u bytes...\n", available);

//         char *buf = asyncHttpRequest->responseLongText();

//         downloadedFile.write(buf, available);
//         downloadedFile.close();
//     }
// }


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
    {"jpeg", "image/jpeg"}};

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

    LittleFS.begin();
    if (LittleFS.exists(path)) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, getContentType(path));
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=3600,public");
        request->send(response);
        return;
    }

    request->send(404, "text/plain", "File not found");

    return;
}
