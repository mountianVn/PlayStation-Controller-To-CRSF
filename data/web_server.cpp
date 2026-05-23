#include "web_server.h"
#include "../src/defines.h"
#include "../config/config_manager.h"
#include "../modules/wifi_manager.h"
#include "../modules/channel_mapper.h"
#include "../drivers/ps_controller.h"
#include "../drivers/crsf_protocol.h"
#include "../drivers/sbus_protocol.h"
#include "../drivers/ppm_protocol.h"
#include "LittleFS.h"
#include "Update.h"

// ============ EXTERNAL INSTANCES ============
extern ConfigManager config;
extern ChannelMapper channelMapper;
extern PSController psController;
extern CRSFProtocol crsfProtocol;
extern SBUSProtocol sbusProtocol;
extern PPMProtocol ppmProtocol;
extern WiFiManager wifiManager;

// ============ GLOBAL STATUS ============
SystemStatus systemStatus;

// ============ CONSTRUCTOR ============
WebServer::WebServer() {
    server = nullptr;
    wsServer = nullptr;
    state = WEB_NOT_STARTED;
    wsMessageCount = 0;
    lastStatusUpdate = 0;
    statusUpdateInterval = STATUS_UPDATE_INTERVAL_MS;
    memset(wsClients, 0, sizeof(wsClients));
}

WebServer::~WebServer() {
    end();
}

// ============ BEGIN ============
bool WebServer::begin() {
    DEBUG_PRINTLN("[Web] Starting web server...");
    
    if (server || wsServer) {
        DEBUG_PRINTLN("[Web] Server already running!");
        return true;
    }
    
    // Create web server
    server = new AsyncWebServer(WEB_SERVER_PORT);
    wsServer = new WebSocketsServer(WEBSOCKET_PORT);
    
    // Setup routes
    setupRoutes();
    setupWebSocket();
    
    // Start servers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server->begin();
    wsServer->begin();
    
    state = WEB_RUNNING;
    DEBUG_PRINTF("[Web] Server started on port %d\n", WEB_SERVER_PORT);
    DEBUG_PRINTF("[Web] WebSocket on port %d\n", WEBSOCKET_PORT);
    
    return true;
}

void WebServer::end() {
    if (wsServer) {
        wsServer->stop();
        delete wsServer;
        wsServer = nullptr;
    }
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    state = WEB_NOT_STARTED;
    DEBUG_PRINTLN("[Web] Server stopped");
}

// ============ UPDATE ============
void WebServer::update() {
    if (state != WEB_RUNNING || !wsServer) return;
    
    wsServer->loop();
    
    // Broadcast status periodically
    uint32_t now = millis();
    if (now - lastStatusUpdate >= statusUpdateInterval) {
        lastStatusUpdate = now;
        broadcastStatus();
    }
}

// ============ SETUP ROUTES ============
void WebServer::setupRoutes() {
    // Serve static files from LittleFS/web
    server->serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    
    // API routes
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPIStatus());
    });
    
    server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPIConfig());
    });
    
    server->on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPISystem());
    });
    
    server->on("/api/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("body", true)) {
            String body = request->getParam("body", true)->value();
            bool success = handleAPISaveConfig(body);
            request->send(success ? 200 : 400, "application/json", 
                "{\"success\":" + String(success) + "}");
        } else {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"No body\"}");
        }
    });
    
    server->on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPICalibration());
    });
    
    server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPIWifiScan());
    });
    
    server->on("/api/wifi/connect", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
            String ssid = request->getParam("ssid", true)->value();
            String pass = request->getParam("password", true)->value();
            request->send(200, "application/json", handleAPIWifiConnect(ssid, pass));
        } else {
            request->send(400, "application/json", "{\"success\":false}");
        }
    });
    
    server->on("/api/ota", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("url", true)) {
            String url = request->getParam("url", true)->value();
            request->send(200, "application/json", handleAPIOta(url));
        } else {
            request->send(400, "application/json", "{\"success\":false}");
        }
    });
    
    server->on("/api/factoryreset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPIFactoryReset());
    });
    
    server->on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", handleAPIReboot());
        delay(500);
        ESP.restart();
    });
    
    server->on("/api/backup", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String backup = config.exportConfig();
        request->send(200, "application/json", backup);
    });
    
    server->on("/api/restore", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("config", true)) {
            String cfg = request->getParam("config", true)->value();
            bool success = config.importConfig(cfg);
            request->send(200, "application/json", 
                "{\"success\":" + String(success) + "}");
        } else {
            request->send(400, "application/json", "{\"success\":false}");
        }
    });
    
    // OTA firmware upload
    server->on("/api/ota/upload", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!Update.hasError()) {
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(500, "application/json", 
                "{\"success\":false,\"error\":\"Update failed\"}");
        }
    }, [this](AsyncWebServerRequest* request, const String& filename, 
             size_t index, uint8_t* data, size_t len, bool final) {
        if (!index) {
            DEBUG_PRINTF("[Web] OTA: Starting upload of %s\n", filename.c_str());
            Update.begin(UPDATE_SIZE_UNKNOWN);
        }
        Update.write(data, len);
        if (final) {
            DEBUG_PRINTLN("[Web] OTA: Upload complete");
            Update.end(true);
        }
    });
    
    // Catch-all
    server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
}

// ============ SETUP WEBSOCKET ============
void WebServer::setupWebSocket() {
    wsServer->onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
        switch (type) {
            case WStype_DISCONNECTED:
                DEBUG_PRINTF("[WS] Client %d disconnected\n", num);
                wsClients[num].active = false;
                break;
                
            case WStype_CONNECTED:
                DEBUG_PRINTF("[WS] Client %d connected\n", num);
                wsClients[num].active = true;
                wsClients[num].connectedTime = millis();
                break;
                
            case WStype_TEXT:
                handleWebSocketMessage(num, payload, len);
                break;
                
            case WStype_BIN:
                // Binary data not used
                break;
        }
    });
}

// ============ HANDLE WEBSOCKET MESSAGE ============
void WebServer::handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t len) {
    wsMessageCount++;
    
    if (len < 2) return;
    
    uint8_t msgType = payload[0];
    
    switch (msgType) {
        case WS_MSG_STATUS:
            // Client requesting status
            broadcastStatus();
            break;
            
        case WS_MSG_CHANNELS:
            // Send channel data
            {
                StaticJsonDocument<1024> doc;
                JsonArray channels = doc.createNestedArray("channels");
                uint8_t chCount = config.getChannelCount();
                for (int i = 0; i < chCount; i++) {
                    channels.add(channelMapper.getChannel(i));
                }
                String response;
                serializeJson(doc, response);
                wsServer->sendTXT(num, response);
            }
            break;
            
        case WS_MSG_CONFIG_GET:
            wsServer->sendTXT(num, handleAPIConfig());
            break;
            
        case WS_MSG_CONFIG_SET:
            if (len > 1) {
                String json = String((char*)&payload[1]);
                handleAPISaveConfig(json);
                broadcastStatus();
            }
            break;
            
        case WS_MSG_SAVE_CONFIG:
            config.saveConfig();
            wsServer->sendTXT(num, "{\"saved\":true}");
            break;
            
        case WS_MSG_REBOOT:
            wsServer->sendTXT(num, "{\"rebooting\":true}");
            delay(100);
            ESP.restart();
            break;
    }
}

// ============ BROADCAST STATUS ============
void WebServer::broadcastStatus() {
    if (!wsServer || state != WEB_RUNNING) return;
    
    StaticJsonDocument<512> doc;
    
    doc["controllerConnected"] = psController.isConnected();
    doc["controllerType"] = psController.getType();
    doc["batteryLevel"] = psController.getBatteryLevel();
    doc["rssi"] = psController.getRSSI();
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["activeProtocol"] = config.getActiveProtocol();
    doc["crsfEnabled"] = crsfProtocol.isEnabled();
    doc["sbusEnabled"] = sbusProtocol.isEnabled();
    doc["ppmEnabled"] = ppmProtocol.isEnabled();
    doc["crsfFrameCount"] = crsfProtocol.getFrameCount();
    doc["sbusFrameCount"] = sbusProtocol.getFrameCount();
    doc["ppmFrameCount"] = ppmProtocol.getFrameCount();
    
    JsonArray channels = doc.createNestedArray("channels");
    uint8_t chCount = config.getChannelCount();
    for (int i = 0; i < chCount; i++) {
        channels.add(channelMapper.getChannel(i));
    }
    
    String response;
    serializeJson(doc, response);
    
    wsServer->broadcastTXT(response);
}

// ============ API HANDLERS ============
String WebServer::handleAPIStatus() {
    StaticJsonDocument<512> doc;
    
    doc["controllerConnected"] = psController.isConnected();
    doc["controllerType"] = psController.getType();
    doc["controllerName"] = psController.getName();
    doc["batteryLevel"] = psController.getBatteryLevel();
    doc["rssi"] = psController.getRSSI();
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["wifiState"] = wifiManager.getStateString();
    doc["wifiRSSI"] = wifiManager.getRSSI();
    doc["wifiIP"] = wifiManager.getLocalIP().toString();
    doc["activeProtocol"] = config.getActiveProtocol();
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIConfig() {
    return config.exportConfig();
}

String WebServer::handleAPISystem() {
    StaticJsonDocument<256> doc;
    
    doc["firmwareVersion"] = FIRMWARE_VERSION;
    doc["firmwareName"] = FIRMWARE_NAME;
    doc["buildDate"] = BUILD_DATE;
    doc["espModel"] = ESP.getChipModel();
    doc["espRevision"] = ESP.getChipRevision();
    doc["cpuCores"] = ESP.getChipCores();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["psramSize"] = ESP.getPsramSize();
    
    String response;
    serializeJson(doc, response);
    return response;
}

bool WebServer::handleAPISaveConfig(const String& json) {
    return config.importConfig(json);
}

String WebServer::handleAPICalibration() {
    StaticJsonDocument<512> doc;
    CalibrationData* cal = config.getCalibration();
    
    JsonArray minArr = doc.createNestedArray("min");
    JsonArray maxArr = doc.createNestedArray("max");
    JsonArray centerArr = doc.createNestedArray("center");
    
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        minArr.add(cal->min[i]);
        maxArr.add(cal->max[i]);
        centerArr.add(cal->center[i]);
    }
    
    doc["calibrated"] = config.isCalibrated();
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIWifiScan() {
    StaticJsonDocument<2048> doc;
    JsonArray networks = doc.createNestedArray("networks");
    
    int n = wifiManager.scanNetworks();
    if (n > 0) {
        for (int i = 0; i < n && i < 20; i++) {
            JsonObject net = networks.createNestedObject();
            net["ssid"] = wifiManager.getScanResultSSID(i);
            net["rssi"] = wifiManager.getScanResultRSSI(i);
        }
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIWifiConnect(const String& ssid, const String& pass) {
    StaticJsonDocument<128> doc;
    
    if (wifiManager.begin(ssid.c_str(), pass.c_str())) {
        // Save credentials
        SystemConfig* sys = config.getSystemConfig();
        strncpy(sys->wifiSSID, ssid.c_str(), sizeof(sys->wifiSSID) - 1);
        strncpy(sys->wifiPassword, pass.c_str(), sizeof(sys->wifiPassword) - 1);
        config.saveConfig();
        
        doc["success"] = true;
        doc["ip"] = wifiManager.getLocalIP().toString();
    } else {
        doc["success"] = false;
        doc["error"] = "Connection failed";
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIOta(const String& url) {
    StaticJsonDocument<128> doc;
    
    // Start OTA download
    // This is a simplified version - actual implementation would use HTTPClient
    doc["success"] = true;
    doc["message"] = "OTA URL received";
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIFactoryReset() {
    config.resetToDefaults();
    
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    
    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::handleAPIReboot() {
    StaticJsonDocument<64> doc;
    doc["rebooting"] = true;
    
    String response;
    serializeJson(doc, response);
    return response;
}

uint8_t WebServer::getConnectedClients() {
    uint8_t count = 0;
    for (int i = 0; i < WEBSOCKETS_SERVER_MAX_SLOTS; i++) {
        if (wsClients[i].active) count++;
    }
    return count;
}
