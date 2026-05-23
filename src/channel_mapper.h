#ifndef CHANNEL_MAPPER_H
#define CHANNEL_MAPPER_H

#include <Arduino.h>
#include "defines.h"
#include "config_manager.h"

// ============ INPUT STATE STRUCTURES ============
typedef struct {
    int16_t analog[PS_INPUT_COUNT];     // 0-255 for analog inputs
    bool button[PS_INPUT_COUNT];        // true/false for digital buttons
} InputState;

// ============ OUTPUT STATE ============
typedef struct {
    uint16_t channels[MAX_CHANNELS];    // 1000-2000us values
    uint32_t frameCount;
    uint32_t errors;
    uint32_t lastUpdate;
} OutputState;

// ============ CHANNEL MAPPER CLASS ============
class ChannelMapper {
private:
    ConfigManager* config;
    InputState input;
    OutputState output;
    
    // Smoothing buffers
    float smoothedValues[MAX_CHANNELS];
    uint32_t lastUpdateTime[MAX_CHANNELS];
    
    // Internal helpers
    int16_t applyDeadzone(int16_t value, int16_t center, uint8_t deadzone);
    int16_t applyExpo(int16_t value, int16_t center, uint8_t expo);
    int16_t applySmoothing(int16_t value, uint8_t ch, uint8_t smoothing);
    int16_t applyScale(int16_t value, uint16_t scale);
    int16_t applyReverse(int16_t value, bool reversed);
    int16_t mapToOutput(int16_t inputValue, int16_t inMin, int16_t inMax, int16_t outMin, int16_t outMax);
    
public:
    ChannelMapper(ConfigManager* cfg);
    void begin();
    void update();
    
    // Input setters
    void setAnalog(uint8_t index, int16_t value);
    void setButton(uint8_t index, bool pressed);
    void setAllInputs(const InputState& inputs);
    
    // Output getters
    uint16_t getChannel(uint8_t ch);
    uint16_t* getAllChannels() { return output.channels; }
    uint32_t getFrameCount() { return output.frameCount; }
    uint32_t getErrors() { return output.errors; }
    
    // Utility
    void resetSmoothing();
    void processChannel(uint8_t ch);
    int16_t getProcessedInput(ChannelConfig* cfg);
};

// ============ INPUT NAME HELPERS ============
const char* getInputName(uint8_t inputIndex);
const char* getChannelName(uint8_t ch);

#endif // CHANNEL_MAPPER_H
