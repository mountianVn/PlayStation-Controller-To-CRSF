#ifndef CRSF_PROTOCOL_H
#define CRSF_PROTOCOL_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "defines.h"

// ============ CRSF PACKET TYPES ============
#define CRSF_FRAMETYPE_RC_CHANNELS_PACKED 0x16
#define CRSF_FRAMETYPE_LINK_STATISTICS 0x14
#define CRSF_FRAMETYPE_DEVICE_INFO 0x02
#define CRSF_FRAMETYPE_PARAMETER_READ 0x20
#define CRSF_FRAMETYPE_PARAMETER_WRITE 0x21
#define CRSF_FRAMETYPE_COMMAND 0x32

// ============ CRSF DEVICE ADDRESSES ============
#define CRSF_ADDR_FLIGHT_CONTROLLER 0xC8
#define CRSF_ADDR_RADIO 0xEA
#define CRSF_ADDR_TRANSMITTER 0xEE
#define CRSF_ADDR_BROADCAST 0xFF

// ============ CRSF TIMING ============
#define CRSF_TIME_BETWEEN_FRAMES_US 4000  // 4ms = 250Hz max
#define CRSF_PACKET_SIZE 26
#define CRSF_PAYLOAD_SIZE 24

// ============ LINK STATISTICS ============
typedef struct {
    uint8_t uplinkRSSI1;
    uint8_t uplinkRSSI2;
    uint8_t uplinkLQ;
    int8_t uplinkSNR;
    uint8_t antenna;
    uint8_t mode;
    int8_t downlinkRSSI;
    uint8_t downlinkLQ;
    int8_t downlinkSNR;
} __attribute__((packed)) LinkStatistics;

// ============ CRSF CLASS ============
class CRSFProtocol {
private:
    HardwareSerial* serial;
    uint8_t channelData[22];  // 16 channels * 11 bits / 8 = 22 bytes
    uint8_t packetBuffer[64];
    uint32_t lastFrameTime;
    uint32_t frameCount;
    uint32_t errorCount;
    bool initialized;
    bool enabled;
    
    // Link stats (from ELRS)
    LinkStatistics linkStats;
    
    void buildRCChannelsPacket();
    uint8_t calculateCRC(uint8_t* data, size_t len);
    
public:
    CRSFProtocol(HardwareSerial* uart);
    ~CRSFProtocol();
    
    bool begin(uint32_t baudrate = CRSF_BAUDRATE);
    void end();
    void setEnabled(bool en);
    bool isEnabled() { return enabled; }
    bool isInitialized() { return initialized; }
    
    void sendChannels(const uint16_t* channels, uint8_t count);
    
    // Status
    uint32_t getFrameCount() { return frameCount; }
    uint32_t getErrorCount() { return errorCount; }
    uint32_t getFrameRate();
    uint32_t getLastFrameTime() { return lastFrameTime; }
    LinkStatistics* getLinkStats() { return &linkStats; }
    
    // Packet generation
    void buildPacket(uint8_t type, const uint8_t* payload, size_t payloadLen);
};

#endif // CRSF_PROTOCOL_H
