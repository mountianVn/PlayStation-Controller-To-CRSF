#include "wifi_manager.h"

// ============ GLOBAL INSTANCE ============
WiFiManager wifiManager;

// ============ CONSTRUCTOR ============
WiFiManager::WiFiManager() {
    state = WIFI_IDLE;
    configured = false;
    connectStartTime = 0;
    reconnectInterval = STA_RECONNECT_INTERVAL_MS;
    lastReconnectAttempt = 0;
    apIP = AP_IP_ADDRESS;
}

// ============ BEGIN (STATION MODE) ============
bool WiFiManager::begin(const char* s, const char* p) {
    if (!s || strlen(s) == 0) {
        DEBUG_PRINTLN("[WiFi] No SSID provided!");
        return false;
    }
    
    ssid = s;
    password = p ? p : "";
    configured = true;
    
    DEBUG_PRINTF("[WiFi] Connecting to %s...\n", ssid.c_str());
    
    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
        delay(100);
    }
    
    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    
    // Store start time
    connectStartTime = millis();
    state = WIFI_CONNECTING;
    
    // Start connection
    if (password.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
    } else {
        WiFi.begin(ssid.c_str());
    }
    
    // Wait for connection with timeout
    uint32_t startWait = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startWait < WIFI_CONNECT_TIMEOUT_MS) {
        delay(100);
        DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN();
    
    if (WiFi.status() == WL_CONNECTED) {
        localIP = WiFi.localIP();
        state = WIFI_CONNECTED;
        DEBUG_PRINTF("[WiFi] Connected! IP: %s\n", localIP.toString().c_str());
        DEBUG_PRINTF("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    }
    
    DEBUG_PRINTLN("[WiFi] Connection failed!");
    state = WIFI_ERROR;
    return false;
}

// ============ BEGIN (AP MODE) ============
bool WiFiManager::beginAP(const char* apSsid, const char* apPass) {
    DEBUG_PRINTF("[WiFi] Starting AP mode: %s\n", apSsid);
    
    WiFi.mode(WIFI_AP);
    
    bool success;
    if (apPass && strlen(apPass) >= 8) {
        success = WiFi.softAP(apSsid, apPass, AP_CHANNEL);
    } else {
        success = WiFi.softAP(apSsid);
    }
    
    if (!success) {
        DEBUG_PRINTLN("[WiFi] AP start failed!");
        state = WIFI_ERROR;
        return false;
    }
    
    delay(100);
    
    localIP = WiFi.softAPIP();
    state = WIFI_AP_MODE;
    DEBUG_PRINTF("[WiFi] AP started. IP: %s\n", localIP.toString().c_str());
    
    return true;
}

// ============ END ============
void WiFiManager::end() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    state = WIFI_IDLE;
    configured = false;
}

// ============ DISCONNECT ============
void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    state = WIFI_DISCONNECTED;
}

// ============ RECONNECT ============
bool WiFiManager::reconnect() {
    if (!configured || ssid.length() == 0) {
        DEBUG_PRINTLN("[WiFi] No credentials for reconnection");
        return false;
    }
    
    uint32_t now = millis();
    if (now - lastReconnectAttempt < reconnectInterval) {
        return false;
    }
    lastReconnectAttempt = now;
    
    DEBUG_PRINTLN("[WiFi] Attempting reconnection...");
    return begin(ssid.c_str(), password.c_str());
}

// ============ GET STATE STRING ============
const char* WiFiManager::getStateString() {
    switch (state) {
        case WIFI_IDLE: return "Idle";
        case WIFI_CONNECTING: return "Connecting...";
        case WIFI_CONNECTED: return "Connected";
        case WIFI_AP_MODE: return "AP Mode";
        case WIFI_DISCONNECTED: return "Disconnected";
        case WIFI_ERROR: return "Error";
        default: return "Unknown";
    }
}

// ============ GET CONNECTION DURATION ============
uint32_t WiFiManager::getConnectionDuration() {
    if (state == WIFI_CONNECTED || state == WIFI_AP_MODE) {
        return millis() - connectStartTime;
    }
    return 0;
}

// ============ LOAD CREDENTIALS ============
void WiFiManager::loadCredentials(const char* s, const char* p) {
    if (s) ssid = s;
    if (p) password = p;
    configured = ssid.length() > 0;
}

// ============ CLEAR CREDENTIALS ============
void WiFiManager::clearCredentials() {
    ssid = "";
    password = "";
    configured = false;
}

// ============ SCAN NETWORKS ============
int WiFiManager::scanNetworks() {
    DEBUG_PRINTLN("[WiFi] Scanning networks...");
    int n = WiFi.scanNetworks(true);
    
    // Wait for scan to complete
    int timeout = 10000;
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && timeout > 0) {
        delay(100);
        timeout -= 100;
    }
    
    return WiFi.scanComplete();
}

String WiFiManager::getScanResultSSID(int index) {
    return WiFi.SSID(index);
}

int32_t WiFiManager::getScanResultRSSI(int index) {
    return WiFi.RSSI(index);
}

// ============ WIFI EVENT HANDLER ============
void WiFiManager::handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            state = WIFI_CONNECTED;
            localIP = WiFi.localIP();
            DEBUG_PRINTF("[WiFi] Got IP: %s\n", localIP.toString().c_str());
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            state = WIFI_DISCONNECTED;
            DEBUG_PRINTLN("[WiFi] Station disconnected");
            break;
            
        case SYSTEM_EVENT_AP_STACONNECTED:
            DEBUG_PRINTF("[WiFi] Client connected to AP\n");
            break;
            
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            DEBUG_PRINTF("[WiFi] Client disconnected from AP\n");
            break;
            
        default:
            break;
    }
}
