#include "crsf_protocol.h"

// ============ CONSTRUCTOR ============
CRSFProtocol::CRSFProtocol(HardwareSerial* uart) : serial(uart) {
    initialized = false;
    enabled = false;
    frameCount = 0;
    errorCount = 0;
    lastFrameTime = 0;
    memset(channelData, 0, sizeof(channelData));
    memset(&linkStats, 0, sizeof(LinkStatistics));
}

CRSFProtocol::~CRSFProtocol() {
    end();
}

// ============ INITIALIZATION ============
bool CRSFProtocol::begin(uint32_t baudrate) {
    DEBUG_PRINTLN("[CRSF] Initializing...");
    
    if (!serial) {
        DEBUG_PRINTLN("[CRSF] Error: Serial port not configured!");
        return false;
    }
    
    serial->begin(baudrate, SERIAL_8N1);
    
    // Flush buffers
    while (serial->available()) {
        serial->read();
    }
    
    enabled = true;
    initialized = true;
    frameCount = 0;
    errorCount = 0;
    lastFrameTime = micros();
    
    DEBUG_PRINTF("[CRSF] Started at %d baud\n", baudrate);
    return true;
}

void CRSFProtocol::end() {
    if (serial && initialized) {
        serial->flush();
        serial->end();
    }
    initialized = false;
    enabled = false;
}

void CRSFProtocol::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINTF("[CRSF] %s\n", enabled ? "Enabled" : "Disabled");
}

// ============ CHANNEL DATA PACKING ============
// CRSF uses 11-bit channel data packing
// Channel value range: 172 - 1811 (CRSFPwmConverter)
void CRSFProtocol::buildRCChannelsPacket() {
    // Packet structure:
    // [DeviceAddress][FrameSize][Type][Payload...][CRC]
    
    packetBuffer[0] = CRSF_ADDR_FLIGHT_CONTROLLER;
    packetBuffer[1] = CRSF_PAYLOAD_SIZE + 2;  // payload + type + crc
    packetBuffer[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
    
    // Pack 16 channels (11 bits each) into 22 bytes
    // Channel value: 0-2047 (CRSF format)
    // Standard RC: 1000-2000us -> 0-2047
    
    for (int ch = 0; ch < 16; ch++) {
        uint16_t pwmValue = channelData[ch * 2] | (channelData[ch * 2 + 1] << 8);
        
        // Convert 1000-2000 to 0-2047
        uint16_t crsfValue = map(pwmValue, 1000, 2000, 172, 1811);
        crsfValue = constrain(crsfValue, 172, 1811);
        
        // Pack into channelData buffer (CRSF format)
        int bitPos = ch * 11;
        int bytePos = bitPos / 8;
        int bitOffset = bitPos % 8;
        
        packetBuffer[3 + bytePos] = crsfValue & 0xFF;
        if (bitOffset <= 5) {
            packetBuffer[3 + bytePos] |= (crsfValue >> (bitOffset + 3)) & 0xFF;
            packetBuffer[4 + bytePos] = crsfValue >> (bitOffset - 5);
            if (bitOffset >= 6) {
                packetBuffer[4 + bytePos] |= (crsfValue >> (bitOffset - 5)) & 0xFF;
            }
        } else {
            packetBuffer[3 + bytePos] |= (crsfValue << (bitOffset - 8)) & 0xFF;
            packetBuffer[4 + bytePos] = crsfValue >> (16 - bitOffset);
        }
    }
    
    // Calculate and add CRC
    uint8_t crc = calculateCRC(&packetBuffer[2], CRSF_PAYLOAD_SIZE);
    packetBuffer[CRSF_PACKET_SIZE - 1] = crc;
}

// ============ SEND CHANNELS ============
void CRSFProtocol::sendChannels(const uint16_t* channels, uint8_t count) {
    if (!enabled || !initialized) return;
    
    // Store channel data
    uint8_t chCount = min(count, (uint8_t)16);
    for (int i = 0; i < chCount; i++) {
        channelData[i * 2] = channels[i] & 0xFF;
        channelData[i * 2 + 1] = (channels[i] >> 8) & 0xFF;
    }
    
    // Fill remaining with neutral
    for (int i = chCount; i < 16; i++) {
        channelData[i * 2] = 0xA4;  // 1500 in little endian
        channelData[i * 2 + 1] = 0x05;
    }
    
    // Build packet
    buildRCChannelsPacket();
    
    // Send via serial
    serial->write(packetBuffer, CRSF_PACKET_SIZE);
    serial->flush();  // Ensure all bytes are sent
    
    frameCount++;
    lastFrameTime = micros();
}

// ============ CRC CALCULATION ============
uint8_t CRSFProtocol::calculateCRC(uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

// ============ FRAME RATE ============
uint32_t CRSFProtocol::getFrameRate() {
    uint32_t elapsed = micros() - lastFrameTime;
    if (elapsed > 0 && frameCount > 0) {
        // Estimate based on last frame
        return 1000000 / max(elapsed, (uint32_t)1);
    }
    return 0;
}

// ============ ADVANCED PACKET BUILDING ============
void CRSFProtocol::buildPacket(uint8_t type, const uint8_t* payload, size_t payloadLen) {
    if (payloadLen > 60) return;  // Safety limit
    
    packetBuffer[0] = CRSF_ADDR_FLIGHT_CONTROLLER;
    packetBuffer[1] = payloadLen + 2;
    packetBuffer[2] = type;
    
    memcpy(&packetBuffer[3], payload, payloadLen);
    
    uint8_t crc = calculateCRC(&packetBuffer[2], payloadLen + 1);
    packetBuffer[payloadLen + 3] = crc;
    
    serial->write(packetBuffer, payloadLen + 4);
}
