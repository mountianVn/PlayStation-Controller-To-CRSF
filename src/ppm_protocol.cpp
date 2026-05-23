#include "ppm_protocol.h"

// ============ STATIC VARIABLES FOR ISR ============
static volatile uint32_t g_ppmPeriod = PPM_FRAME_US;
static volatile bool g_ppmEnabled = false;
static volatile uint8_t g_ppmChannels = 8;
static volatile uint16_t g_ppmData[24];
static volatile bool g_ppmInverted = false;

// ISR-safe channel buffer
static volatile uint8_t g_currentChannel = 0;
static volatile uint32_t g_lastPulseTime = 0;
static volatile bool g_ppmState = false;

// ============ CONSTRUCTOR ============
PPMProtocol::PPMProtocol() {
    outputPin = 0xFF;
    channelCount = 0;
    frameCount = 0;
    lastFrameTime = 0;
    initialized = false;
    enabled = false;
    frameRate = 50;
    maxChannels = 24;
    inverted = false;
    channelBuffer = nullptr;
}

PPMProtocol::~PPMProtocol() {
    end();
}

// ============ INITIALIZATION ============
bool PPMProtocol::begin(uint8_t pin, uint8_t channels, bool inv) {
    DEBUG_PRINTLN("[PPM] Initializing...");
    
    if (pin > 48) {
        DEBUG_PRINTLN("[PPM] Error: Invalid pin!");
        return false;
    }
    
    outputPin = pin;
    channelCount = channels;
    inverted = inv;
    frameRate = 50;
    
    // Allocate buffer
    channelBuffer = new volatile uint16_t[maxChannels];
    if (!channelBuffer) {
        DEBUG_PRINTLN("[PPM] Error: Memory allocation failed!");
        return false;
    }
    
    // Initialize all channels to neutral
    for (int i = 0; i < maxChannels; i++) {
        channelBuffer[i] = PPM_MIN_US;
        g_ppmData[i] = PPM_MIN_US;
    }
    
    // Configure pin
    pinMode(outputPin, OUTPUT);
    digitalWrite(outputPin, inverted ? HIGH : LOW);
    
    // Calculate frame time
    g_ppmPeriod = calculatePPMFrameTime(channelCount, frameRate);
    g_ppmChannels = channelCount;
    g_ppmInverted = inverted;
    
    enabled = true;
    initialized = true;
    frameCount = 0;
    lastFrameTime = micros();
    
    DEBUG_PRINTF("[PPM] Started on pin %d, %d channels, inverted=%d\n", pin, channels, inverted);
    return true;
}

void PPMProtocol::end() {
    if (initialized) {
        detachInterrupt(outputPin);
        pinMode(outputPin, INPUT);
    }
    
    if (channelBuffer) {
        delete[] channelBuffer;
        channelBuffer = nullptr;
    }
    
    initialized = false;
    enabled = false;
}

void PPMProtocol::setEnabled(bool en) {
    enabled = en;
    g_ppmEnabled = en;
    DEBUG_PRINTF("[PPM] %s\n", enabled ? "Enabled" : "Disabled");
}

// ============ SEND CHANNELS ============
void PPMProtocol::sendChannels(const uint16_t* channels, uint8_t count) {
    if (!enabled || !initialized) return;
    
    uint8_t chCount = min(count, maxChannels);
    
    // Copy to ISR buffer
    noInterrupts();
    for (int i = 0; i < chCount; i++) {
        g_ppmData[i] = constrain(channels[i], PPM_MIN_US, PPM_MAX_US);
    }
    // Fill rest with neutral
    for (int i = chCount; i < maxChannels; i++) {
        g_ppmData[i] = CHANNEL_MID_US;
    }
    g_ppmChannels = chCount;
    interrupts();
    
    frameCount++;
    lastFrameTime = micros();
}

void PPMProtocol::setFrameRate(uint32_t fps) {
    frameRate = constrain(fps, 50, 400);
    g_ppmPeriod = calculatePPMFrameTime(g_ppmChannels, frameRate);
}

// ============ FRAME TIME CALCULATION ============
uint32_t calculatePPMFrameTime(uint8_t channels, uint32_t frameRate) {
    if (frameRate == 0) frameRate = 50;
    
    uint32_t frameTimeUs = 1000000 / frameRate;
    
    // PPM frame time must accommodate all channels
    // Each channel: pulse (400us) + width (1000-2000us) = 1400-2400us
    // Default: 8 channels @ 2000us each = 16000us + 400us sync = ~16400us
    // So frame rate of 50Hz (20000us) works, but 60Hz (16666us) may be tight
    
    uint32_t minFrameTime = (channels * PPM_MAX_US) + PPM_PULSE_US + 1000;
    
    if (frameTimeUs < minFrameTime) {
        frameTimeUs = minFrameTime;
    }
    
    return frameTimeUs;
}

// ============ PPM TIMING HELPER ============
// For ESP32, we use ledcWrite for PWM-based PPM generation
// This provides precise timing without CPU overhead
