#include "channel_mapper.h"

// ============ CONSTRUCTOR ============
ChannelMapper::ChannelMapper(ConfigManager* cfg) : config(cfg) {
    memset(&input, 0, sizeof(InputState));
    memset(&output, 0, sizeof(OutputState));
    memset(smoothedValues, 0, sizeof(smoothedValues));
    memset(lastUpdateTime, 0, sizeof(lastUpdateTime));
    
    // Initialize all channels to mid value
    for (int i = 0; i < MAX_CHANNELS; i++) {
        output.channels[i] = CHANNEL_MID_US;
        smoothedValues[i] = CHANNEL_MID_US;
    }
}

// ============ INITIALIZATION ============
void ChannelMapper::begin() {
    DEBUG_PRINTLN("[Mapper] Initializing channel mapper...");
    resetSmoothing();
    DEBUG_PRINTLN("[Mapper] Channel mapper ready");
}

// ============ MAIN UPDATE ============
void ChannelMapper::update() {
    uint8_t channelCount = config->getChannelCount();
    
    for (int ch = 0; ch < channelCount; ch++) {
        processChannel(ch);
    }
    
    output.frameCount++;
    output.lastUpdate = millis();
}

// ============ INPUT SETTERS ============
void ChannelMapper::setAnalog(uint8_t index, int16_t value) {
    if (index < PS_INPUT_COUNT) {
        input.analog[index] = constrain(value, 0, 255);
    }
}

void ChannelMapper::setButton(uint8_t index, bool pressed) {
    if (index < PS_INPUT_COUNT) {
        input.button[index] = pressed;
    }
}

void ChannelMapper::setAllInputs(const InputState& inputs) {
    memcpy(&input, &inputs, sizeof(InputState));
}

// ============ PROCESS CHANNEL ============
void ChannelMapper::processChannel(uint8_t ch) {
    ChannelConfig* cfg = config->getChannel(ch);
    if (!cfg || !cfg->enabled) {
        output.channels[ch] = CHANNEL_MID_US;
        return;
    }
    
    // Get raw input value
    int16_t rawValue = getProcessedInput(cfg);
    
    // Apply all transformations
    int16_t value = rawValue;
    
    // Apply deadzone
    int16_t center = cfg->centerValue;
    value = applyDeadzone(value, center, cfg->deadzone);
    
    // Apply expo
    value = applyExpo(value, center, cfg->expo);
    
    // Apply scale
    value = applyScale(value, cfg->scale);
    
    // Apply reverse
    value = applyReverse(value, cfg->reversed);
    
    // Apply smoothing
    value = applySmoothing(value, ch, cfg->smoothing);
    
    // Map to output range
    value = mapToOutput(value, 0, 255, cfg->minValue, cfg->maxValue);
    
    // Clamp to valid range
    output.channels[ch] = constrain(value, cfg->minValue, cfg->maxValue);
}

int16_t ChannelMapper::getProcessedInput(ChannelConfig* cfg) {
    if (!cfg) return 128;
    
    switch (cfg->inputSource) {
        case INPUT_ANALOG:
            if (cfg->inputIndex < PS_INPUT_COUNT) {
                return input.analog[cfg->inputIndex];
            }
            break;
            
        case INPUT_BUTTON_DIGITAL:
            if (cfg->inputIndex < PS_INPUT_COUNT) {
                return input.button[cfg->inputIndex] ? 255 : 0;
            }
            break;
            
        case INPUT_BUTTON_ANALOG:
            if (cfg->inputIndex < PS_INPUT_COUNT) {
                return input.analog[cfg->inputIndex];
            }
            break;
            
        case INPUT_DPAD:
            if (cfg->inputIndex < PS_INPUT_COUNT) {
                return input.button[cfg->inputIndex] ? 255 : 0;
            }
            break;
    }
    
    return 128; // Default center
}

// ============ APPLY TRANSFORMATIONS ============
int16_t ChannelMapper::applyDeadzone(int16_t value, int16_t center, uint8_t deadzone) {
    if (deadzone == 0) return value;
    
    int16_t dz = map(deadzone, 0, 100, 0, 128);
    int16_t diff = value - center;
    
    if (abs(diff) < dz) {
        return center;
    }
    
    // Rescale the remaining range
    int16_t sign = (diff > 0) ? 1 : -1;
    int16_t newDiff = abs(diff) - dz;
    int16_t maxVal = 128 - dz;
    
    return center + sign * map(newDiff, 0, maxVal, 0, 128);
}

int16_t ChannelMapper::applyExpo(int16_t value, int16_t center, uint8_t expo) {
    if (expo == 0) return value;
    
    int16_t diff = value - center;
    float factor = expo / 100.0;
    float normalized = diff / 128.0;
    
    // Exponential curve
    float expoValue = normalized * normalized * normalized * factor +
                      normalized * (1.0 - factor);
    
    return center + (int16_t)(expoValue * 128.0);
}

int16_t ChannelMapper::applySmoothing(int16_t value, uint8_t ch, uint8_t smoothing) {
    if (smoothing == 0) {
        smoothedValues[ch] = value;
        return value;
    }
    
    float alpha = smoothing / 100.0;
    smoothedValues[ch] = smoothedValues[ch] * (1.0 - alpha) + value * alpha;
    
    return (int16_t)smoothedValues[ch];
}

int16_t ChannelMapper::applyScale(int16_t value, uint16_t scale) {
    if (scale == 100) return value;
    
    int16_t center = 128;
    int16_t diff = value - center;
    float scaledDiff = diff * (scale / 100.0);
    
    return center + (int16_t)scaledDiff;
}

int16_t ChannelMapper::applyReverse(int16_t value, bool reversed) {
    if (!reversed) return value;
    return 255 - value + 0; // 255 - value gives inverted (center=128, value becomes 255-value)
}

int16_t ChannelMapper::mapToOutput(int16_t inputValue, int16_t inMin, int16_t inMax, int16_t outMin, int16_t outMax) {
    // Normalize input to 0-1
    float normalized = (float)(inputValue - inMin) / (float)(inMax - inMin);
    // Map to output range
    return outMin + (int16_t)(normalized * (outMax - outMin));
}

// ============ UTILITY ============
void ChannelMapper::resetSmoothing() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        smoothedValues[i] = CHANNEL_MID_US;
        lastUpdateTime[i] = 0;
    }
}

uint16_t ChannelMapper::getChannel(uint8_t ch) {
    if (ch < MAX_CHANNELS) {
        return output.channels[ch];
    }
    return CHANNEL_MID_US;
}

// ============ INPUT NAME HELPERS ============
const char* getInputName(uint8_t inputIndex) {
    static const char* names[PS_INPUT_COUNT] = {
        "SELECT", "L3", "R3", "START", "DPAD_UP", "DPAD_RIGHT", "DPAD_DOWN", "DPAD_LEFT",
        "L2", "R2", "L1", "R1", "TRIANGLE", "CIRCLE", "CROSS", "SQUARE", "PS",
        "ANALOG_LX", "ANALOG_LY", "ANALOG_RX", "ANALOG_RY", "ANALOG_L2", "ANALOG_R2"
    };
    
    if (inputIndex < PS_INPUT_COUNT) {
        return names[inputIndex];
    }
    return "UNKNOWN";
}

const char* getChannelName(uint8_t ch) {
    static char name[16];
    snprintf(name, sizeof(name), "CH%d", ch + 1);
    return name;
}
