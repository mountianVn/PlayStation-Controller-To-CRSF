#include "sbus_protocol.h"

// ============ CONSTRUCTOR ============
SBUSProtocol::SBUSProtocol(HardwareSerial* uart) : serial(uart) {
    initialized = false;
    enabled = false;
    inverted = true;
    frameCount = 0;
    errorCount = 0;
    lastFrameTime = 0;
    flags = 0;
    memset(sbusData, 0, sizeof(sbusData));
}

SBUSProtocol::~SBUSProtocol() {
    end();
}

// ============ INITIALIZATION ============
bool SBUSProtocol::begin(uint32_t baudrate, bool inv) {
    DEBUG_PRINTLN("[SBUS] Initializing...");
    
    if (!serial) {
        DEBUG_PRINTLN("[SBUS] Error: Serial port not configured!");
        return false;
    }
    
    inverted = inv;
    
    // SBUS is 8N2 with inverted signal
    serial->begin(baudrate, SERIAL_8N2);
    
    if (inverted) {
        // For ESP32, we need to use inverted mode via GPIO matrix
        // This is typically handled at the pin level
        DEBUG_PRINTLN("[SBUS] Using inverted signal mode");
    }
    
    // Flush buffers
    while (serial->available()) {
        serial->read();
    }
    
    enabled = true;
    initialized = true;
    frameCount = 0;
    errorCount = 0;
    lastFrameTime = micros();
    
    DEBUG_PRINTF("[SBUS] Started at %d baud, inverted=%d\n", baudrate, inverted);
    return true;
}

void SBUSProtocol::end() {
    if (serial && initialized) {
        serial->flush();
        serial->end();
    }
    initialized = false;
    enabled = false;
}

void SBUSProtocol::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINTF("[SBUS] %s\n", enabled ? "Enabled" : "Disabled");
}

void SBUSProtocol::setInverted(bool inv) {
    inverted = inv;
}

// ============ CHANNEL VALUE MAPPING ============
uint16_t SBUSProtocol::mapToSBUS(uint16_t pwmValue) {
    // Standard RC: 1000-2000us -> SBUS: 173-1811
    // SBUS uses 11-bit resolution (0-2047)
    uint16_t sbus = map(pwmValue, 1000, 2000, SBUS_DIGITAL_CHANNEL_MIN, SBUS_DIGITAL_CHANNEL_MAX);
    return constrain(sbus, SBUS_DIGITAL_CHANNEL_MIN, SBUS_DIGITAL_CHANNEL_MAX);
}

// ============ BUILD SBUS FRAME ============
// SBUS Frame format (25 bytes):
// Byte 0: 0x0F (Start byte)
// Bytes 1-22: 16 channels (11 bits each)
// Byte 23: Flags (CH17, CH18, frame lost, failsafe)
// Byte 24: 0x00 (End byte)
void SBUSProtocol::buildFrame(const uint16_t* channels) {
    memset(sbusData, 0, sizeof(sbusData));
    
    sbusData[0] = SBUS_STARTBYTE;
    
    // Pack 16 channels into 22 bytes (11 bits per channel)
    // Ch1: bits 0-10 (bytes 1-2)
    // Ch2: bits 11-21 (bytes 2-4)
    // etc.
    
    for (int ch = 0; ch < SBUS_CHANNEL_COUNT; ch++) {
        uint16_t sbusValue = mapToSBUS(channels[ch]);
        
        int byteIndex = 1 + (ch * 11) / 8;
        int bitIndex = (ch * 11) % 8;
        
        sbusData[byteIndex] |= (sbusValue << bitIndex) & 0xFF;
        sbusData[byteIndex + 1] |= (sbusValue >> (8 - bitIndex)) & 0xFF;
        sbusData[byteIndex + 2] |= (sbusValue >> (16 - bitIndex)) & 0xFF;
    }
    
    // Flags byte
    sbusData[23] = flags;
    
    // End byte
    sbusData[24] = SBUS_ENDBYTE;
}

// ============ SEND CHANNELS ============
void SBUSProtocol::sendChannels(const uint16_t* channels, uint8_t count) {
    if (!enabled || !initialized) return;
    
    uint8_t chCount = min(count, (uint8_t)SBUS_CHANNEL_COUNT);
    
    // Build frame
    buildFrame(channels);
    
    // Send via serial
    serial->write(sbusData, SBUS_FRAME_SIZE);
    serial->flush();
    
    frameCount++;
    lastFrameTime = micros();
}

// ============ FAILSAFE ============
void SBUSProtocol::setFailsafe(bool active) {
    if (active) {
        flags |= SBUS_FLAG_FAILSAFE;
    } else {
        flags &= ~SBUS_FLAG_FAILSAFE;
    }
}

void SBUSProtocol::setFrameLost(bool lost) {
    if (lost) {
        flags |= SBUS_FLAG_FRAME_LOST;
    } else {
        flags &= ~SBUS_FLAG_FRAME_LOST;
    }
}

// ============ FRAME RATE ============
uint32_t SBUSProtocol::getFrameRate() {
    uint32_t elapsed = micros() - lastFrameTime;
    if (elapsed > 0 && frameCount > 0) {
        return 1000000 / max(elapsed, (uint32_t)1);
    }
    return 0;
}
