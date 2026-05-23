#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "defines.h"

// ============ CHANNEL CONFIGURATION ============
typedef struct {
    int8_t inputSource;       // INPUT_ANALOG, INPUT_BUTTON_DIGITAL, etc.
    int8_t inputIndex;        // PS button/analog index
    bool reversed;            // Reverse direction
    int16_t minValue;         // Output min (typically 1000)
    int16_t maxValue;         // Output max (typically 2000)
    int16_t centerValue;      // Center point for analog inputs
    uint8_t deadzone;         // Deadzone percentage (0-100)
    uint8_t expo;             // Expo percentage (0-100)
    uint8_t smoothing;        // Smoothing factor (0-100)
    uint16_t scale;           // Scale percentage (0-200, 100 = normal)
    bool enabled;             // Channel enabled
} ChannelConfig;

// ============ PROTOCOL CONFIGURATION ============
typedef struct {
    bool enabled;
    uint32_t baudrate;
    uint8_t txPin;
    uint8_t rxPin;
    bool inverted;
    uint8_t frameRate;        // Hz (e.g., 50, 100)
} ProtocolConfig;

// ============ SYSTEM CONFIGURATION ============
typedef struct {
    char deviceName[32];
    char wifiSSID[32];
    char wifiPassword[64];
    bool apMode;
    uint8_t activeProtocol;   // PROTOCOL_CRSF, SBUS, or PPM
    uint8_t channelCount;     // 8-24 channels
    bool debugEnabled;
    bool ledEnabled;
    uint8_t statusLED;
} SystemConfig;

// ============ CALIBRATION DATA ============
typedef struct {
    int16_t min[PS_INPUT_COUNT];
    int16_t max[PS_INPUT_COUNT];
    int16_t center[PS_INPUT_COUNT];
    bool calibrated;
} CalibrationData;

// ============ MAIN CONFIG CLASS ============
class ConfigManager {
private:
    Preferences prefs;
    StaticJsonDocument<4096> jsonDoc;
    
    ChannelConfig channels[MAX_CHANNELS];
    ProtocolConfig crsfConfig;
    ProtocolConfig sbusConfig;
    ProtocolConfig ppmConfig;
    SystemConfig systemConfig;
    CalibrationData calibration;
    
    bool loadChannelConfig(uint8_t ch);
    bool saveChannelConfig(uint8_t ch);
    void loadDefaultChannelConfig(uint8_t ch, uint8_t inputIdx);
    void loadDefaultSystemConfig();
    void loadDefaultProtocols();
    
public:
    ConfigManager();
    bool begin();
    bool loadConfig();
    bool saveConfig();
    bool resetToDefaults();
    
    // Channel operations
    ChannelConfig* getChannel(uint8_t ch) { return &channels[ch < MAX_CHANNELS ? ch : 0]; }
    bool setChannelConfig(uint8_t ch, ChannelConfig* cfg);
    bool copyChannelConfig(uint8_t srcCh, uint8_t dstCh);
    bool resetChannelConfig(uint8_t ch);
    uint8_t getChannelCount() { return systemConfig.channelCount; }
    void setChannelCount(uint8_t count) { systemConfig.channelCount = count < 8 ? 8 : (count > MAX_CHANNELS ? MAX_CHANNELS : count); }
    
    // Protocol operations
    ProtocolConfig* getProtocolConfig(uint8_t proto) {
        switch(proto) {
            case PROTOCOL_CRSF: return &crsfConfig;
            case PROTOCOL_SBUS: return &sbusConfig;
            case PROTOCOL_PPM: return &ppmConfig;
            default: return nullptr;
        }
    }
    bool setProtocolConfig(uint8_t proto, ProtocolConfig* cfg);
    uint8_t getActiveProtocol() { return systemConfig.activeProtocol; }
    void setActiveProtocol(uint8_t proto) { systemConfig.activeProtocol = proto; }
    
    // System operations
    SystemConfig* getSystemConfig() { return &systemConfig; }
    bool setSystemConfig(SystemConfig* cfg);
    
    // Calibration
    CalibrationData* getCalibration() { return &calibration; }
    bool setCalibration(uint8_t inputIdx, int16_t min, int16_t max, int16_t center);
    bool isCalibrated() { return calibration.calibrated; }
    
    // JSON export/import
    String exportConfig();
    bool importConfig(const String& json);
    bool backupConfig(const String& filename);
    bool restoreConfig(const String& filename);
};

#endif // CONFIG_MANAGER_H
