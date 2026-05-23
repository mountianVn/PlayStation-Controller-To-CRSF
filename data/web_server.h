#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "defines.h"

// ============ WEB SERVER STATES ============
enum WebServerState {
    WEB_NOT_STARTED = 0,
    WEB_RUNNING,
    WEB_ERROR
};

// ============ WEBSOCKET MESSAGE TYPES ============
#define WS_MSG_STATUS 1
#define WS_MSG_CHANNELS 2
#define WS_MSG_CONFIG_GET 3
#define WS_MSG_CONFIG_SET 4
#define WS_MSG_CALIBRATION 5
#define WS_MSG_REBOOT 6
#define WS_MSG_WIFI_SCAN 7
#define WS_MSG_WIFI_CONNECT 8
#define WS_MSG_OTA_START 9
#define WS_MSG_OTA_PROGRESS 10
#define WS_MSG_OTA_COMPLETE 11
#define WS_MSG_OTA_ERROR 12
#define WS_MSG_SAVE_CONFIG 13
#define WS_MSG_FACTORY_RESET 14

// ============ WEBSOCKET CLIENT ============
struct WebSocketClient {
    bool active;
    uint32_t connectedTime;
};

// ============ WEB SERVER CLASS ============
class WebServer {
private:
    AsyncWebServer* server;
    WebSocketsServer* wsServer;
    WebSocketClient wsClients[WEBSOCKETS_SERVER_MAX_SLOTS];
    WebServerState state;
    uint32_t wsMessageCount;
    uint32_t lastStatusUpdate;
    uint32_t statusUpdateInterval;
    
    // Status data
    StaticJsonDocument<512> statusDoc;
    
    // Web handlers
    void setupRoutes();
    void setupWebSocket();
    
    // WebSocket handlers
    void handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t len);
    void broadcastStatus();
    
    // API handlers
    String handleAPIStatus();
    String handleAPIConfig();
    String handleAPISystem();
    bool handleAPISaveConfig(const String& json);
    String handleAPICalibration();
    String handleAPIWifiScan();
    String handleAPIWifiConnect(const String& ssid, const String& pass);
    String handleAPIOta(const String& url);
    String handleAPIFactoryReset();
    String handleAPIReboot();
    
public:
    WebServer();
    ~WebServer();
    
    bool begin();
    void end();
    
    // Loop (call from main loop)
    void update();
    
    // Status
    WebServerState getState() { return state; }
    uint8_t getConnectedClients();
    uint32_t getMessageCount() { return wsMessageCount; }
    
    // Configuration (called from main)
    void setUpdateInterval(uint32_t intervalMs) { statusUpdateInterval = intervalMs; }
};

// ============ STATUS DATA STRUCTURES ============
struct SystemStatus {
    bool controllerConnected;
    uint8_t controllerType;
    int8_t batteryLevel;
    int8_t rssi;
    uint32_t uptime;
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    float cpuUsage;
    uint8_t activeProtocol;
    uint32_t channelUpdateRate;
    uint32_t latencyMs;
    bool crsfEnabled;
    bool sbusEnabled;
    bool ppmEnabled;
    uint32_t crsfFrameCount;
    uint32_t sbusFrameCount;
    uint32_t ppmFrameCount;
};

// ============ EXTERNAL REFERENCES ============
extern SystemStatus systemStatus;
extern WebServer webServer;

#endif // WEB_SERVER_H
