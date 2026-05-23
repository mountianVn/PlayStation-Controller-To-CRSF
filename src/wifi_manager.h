#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "defines.h"

// ============ WIFI STATES ============
enum WiFiState {
    WIFI_IDLE = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_MODE,
    WIFI_DISCONNECTED,
    WIFI_ERROR
};

// ============ WIFI EVENT HANDLER ============
class WiFiManager {
private:
    WiFiState state;
    String ssid;
    String password;
    IPAddress localIP;
    IPAddress apIP;
    uint32_t connectStartTime;
    uint32_t reconnectInterval;
    uint32_t lastReconnectAttempt;
    bool configured;
    
public:
    WiFiManager();
    
    bool begin(const char* ssid, const char* password);
    bool beginAP(const char* ssid, const char* password = nullptr);
    void end();
    void disconnect();
    
    // Connection
    bool isConnected() { return state == WIFI_CONNECTED; }
    bool isAPMode() { return state == WIFI_AP_MODE; }
    bool reconnect();
    void setReconnectInterval(uint32_t intervalMs) { reconnectInterval = intervalMs; }
    
    // Status
    WiFiState getState() { return state; }
    const char* getStateString();
    String getSSID() { return ssid; }
    IPAddress getLocalIP() { return localIP; }
    int8_t getRSSI() { return WiFi.RSSI(); }
    uint32_t getConnectionDuration();
    
    // Configuration
    void loadCredentials(const char* s, const char* p);
    void clearCredentials();
    bool hasCredentials() { return ssid.length() > 0; }
    
    // Scan
    int scanNetworks();
    String getScanResultSSID(int index);
    int32_t getScanResultRSSI(int index);
    
    // Internal
    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
};

// ============ WIFI SCAN RESULTS ============
struct WiFiScanResult {
    String ssid;
    int32_t rssi;
    uint8_t encryption;
    bool hidden;
};

// ============ EXTERNAL VARIABLES ============
extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
