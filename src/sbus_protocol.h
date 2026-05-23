#ifndef SBUS_PROTOCOL_H
#define SBUS_PROTOCOL_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "defines.h"

// ============ SBUS CONSTANTS ============
#define SBUS_CHANNEL_COUNT 16
#define SBUS_FRAME_SIZE 25
#define SBUS_DIGITAL_CHANNEL_MIN 173
#define SBUS_DIGITAL_CHANNEL_MAX 1811

// ============ SBUS FLAGS ============
#define SBUS_FLAG_CH17     0x01
#define SBUS_FLAG_CH18     0x02
#define SBUS_FLAG_FRAME_LOST 0x04
#define SBUS_FLAG_FAILSAFE 0x08

// ============ SBUS CLASS ============
class SBUSProtocol {
private:
    HardwareSerial* serial;
    uint8_t sbusData[SBUS_FRAME_SIZE];
    uint32_t lastFrameTime;
    uint32_t frameCount;
    uint32_t errorCount;
    bool initialized;
    bool enabled;
    bool inverted;
    uint8_t flags;
    
    void buildFrame(const uint16_t* channels);
    uint16_t mapToSBUS(uint16_t pwmValue);
    
public:
    SBUSProtocol(HardwareSerial* uart);
    ~SBUSProtocol();
    
    bool begin(uint32_t baudrate = SBUS_BAUDRATE, bool inverted = true);
    void end();
    void setEnabled(bool en);
    bool isEnabled() { return enabled; }
    bool isInitialized() { return initialized; }
    void setInverted(bool inv);
    
    void sendChannels(const uint16_t* channels, uint8_t count);
    
    // Failsafe
    void setFailsafe(bool active);
    void setFrameLost(bool lost);
    
    // Status
    uint32_t getFrameCount() { return frameCount; }
    uint32_t getErrorCount() { return errorCount; }
    uint32_t getFrameRate();
    uint32_t getLastFrameTime() { return lastFrameTime; }
};

#endif // SBUS_PROTOCOL_H
