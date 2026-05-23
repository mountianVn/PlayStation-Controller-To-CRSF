#ifndef PS_CONTROLLER_H
#define PS_CONTROLLER_H

#include <Arduino.h>
#include "defines.h"
#include "channel_mapper.h"

// ============ CONTROLLER TYPES ============
#define CONTROLLER_TYPE_UNKNOWN 0
#define CONTROLLER_TYPE_DS4 1
#define CONTROLLER_TYPE_DS3 2
#define CONTROLLER_TYPE_XBOX 3
#define CONTROLLER_TYPE_SWITCH 4

// ============ CONTROLLER STATE ============
typedef struct {
    bool connected;
    uint8_t type;
    char name[64];
    int8_t batteryLevel;
    int8_t rssi;
    uint32_t connectedTime;
    uint32_t lastUpdate;
    uint32_t disconnectCount;
} ControllerState;

// ============ PS CONTROLLER CLASS ============
class PSController {
private:
    ControllerState state;
    InputState inputState;
    bool autoReconnect;
    uint32_t lastReconnectAttempt;
    bool controllerReady;
    
    // Bluepad32 integration
    bool bp32Initialized;
    
    // Internal update
    void updateInputState();
    void processControllerData();
    
public:
    PSController();
    ~PSController();
    
    bool begin();
    void end();
    
    // Connection management
    bool isConnected() { return state.connected; }
    bool connect();
    void disconnect();
    void setAutoReconnect(bool enable);
    bool getAutoReconnect() { return autoReconnect; }
    
    // Controller info
    uint8_t getType() { return state.type; }
    const char* getName() { return state.name; }
    int8_t getBatteryLevel() { return state.batteryLevel; }
    int8_t getRSSI() { return state.rssi; }
    uint32_t getConnectedTime() { return state.connected ? millis() - state.connectedTime : 0; }
    uint32_t getDisconnectCount() { return state.disconnectCount; }
    
    // Input reading
    InputState* getInputState() { return &inputState; }
    int16_t getAnalog(uint8_t index);
    bool getButton(uint8_t index);
    
    // Update loop (call in main loop)
    void update();
    
    // Status
    bool isInitialized() { return bp32Initialized; }
    bool isReady() { return controllerReady; }
    ControllerState* getState() { return &state; }
    
    // Debug
    void printStatus();
    void printControllerData();
};

// ============ BUTTON NAME HELPERS ============
const char* getButtonName(uint8_t buttonIndex);

#endif // PS_CONTROLLER_H
