#ifndef PPM_PROTOCOL_H
#define PPM_PROTOCOL_H

#include <Arduino.h>
#include "defines.h"

// ============ PPM CLASS ============
class PPMProtocol {
private:
    uint8_t outputPin;
    uint32_t channelCount;
    uint32_t frameCount;
    uint32_t lastFrameTime;
    bool initialized;
    bool enabled;
    uint32_t frameRate;
    
    volatile uint16_t* channelBuffer;
    uint8_t maxChannels;
    bool inverted;
    
public:
    PPMProtocol();
    ~PPMProtocol();
    
    bool begin(uint8_t pin, uint8_t channels = 8, bool inverted = false);
    void end();
    void setEnabled(bool en);
    bool isEnabled() { return enabled; }
    bool isInitialized() { return initialized; }
    
    void sendChannels(const uint16_t* channels, uint8_t count);
    void setFrameRate(uint32_t fps);
    
    // Status
    uint32_t getFrameCount() { return frameCount; }
    uint32_t getFrameRate() { return frameRate; }
    uint32_t getLastFrameTime() { return lastFrameTime; }
};

// ============ PPM HELPER FUNCTIONS ============
uint32_t calculatePPMFrameTime(uint8_t channels, uint32_t frameRate);
void IRAM_ATTR ppmISR();

#endif // PPM_PROTOCOL_H
