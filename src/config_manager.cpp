#include "config_manager.h"
#include <FS.h>
#include <LittleFS.h>

// ============ DEFAULT CONFIGURATION ============
static const ChannelConfig defaultChannels[MAX_CHANNELS] = {
    // CH1-4: Main analog sticks
    {INPUT_ANALOG, PS_ANALOG_LX, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH1: Left X
    {INPUT_ANALOG, PS_ANALOG_LY, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH2: Left Y
    {INPUT_ANALOG, PS_ANALOG_RX, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH3: Right X
    {INPUT_ANALOG, PS_ANALOG_RY, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH4: Right Y
    // CH5-8: Shoulder buttons
    {INPUT_BUTTON_DIGITAL, PS_BTN_L1, false, 1000, 2000, 1500, 0, 0, 0, 100, true},  // CH5: L1
    {INPUT_BUTTON_DIGITAL, PS_BTN_R1, false, 1000, 2000, 1500, 0, 0, 0, 100, true},  // CH6: R1
    {INPUT_BUTTON_ANALOG, PS_ANALOG_L2, false, 1000, 2000, 1000, 0, 0, 0, 100, true}, // CH7: L2
    {INPUT_BUTTON_ANALOG, PS_ANALOG_R2, false, 1000, 2000, 1000, 0, 0, 0, 100, true}, // CH8: R2
    // CH9-12: Face buttons
    {INPUT_BUTTON_DIGITAL, PS_BTN_TRIANGLE, false, 1000, 2000, 1500, 0, 0, 0, 100, true}, // CH9: Triangle
    {INPUT_BUTTON_DIGITAL, PS_BTN_CIRCLE, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH10: Circle
    {INPUT_BUTTON_DIGITAL, PS_BTN_CROSS, false, 1000, 2000, 1500, 0, 0, 0, 100, true},     // CH11: Cross
    {INPUT_BUTTON_DIGITAL, PS_BTN_SQUARE, false, 1000, 2000, 1500, 0, 0, 0, 100, true},    // CH12: Square
    // CH13-16: D-Pad
    {INPUT_BUTTON_DIGITAL, PS_BTN_DPAD_UP, false, 1000, 2000, 1500, 0, 0, 0, 100, true},     // CH13: DPad Up
    {INPUT_BUTTON_DIGITAL, PS_BTN_DPAD_DOWN, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH14: DPad Down
    {INPUT_BUTTON_DIGITAL, PS_BTN_DPAD_LEFT, false, 1000, 2000, 1500, 0, 0, 0, 100, true},   // CH15: DPad Left
    {INPUT_BUTTON_DIGITAL, PS_BTN_DPAD_RIGHT, false, 1000, 2000, 1500, 0, 0, 0, 100, true},  // CH16: DPad Right
    // CH17-24: Reserved
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
    {INPUT_ANALOG, PS_INPUT_COUNT, false, 1000, 2000, 1500, 0, 0, 0, 100, false},
};

static const ProtocolConfig defaultCRSF = {
    .enabled = true,
    .baudrate = 115200,
    .txPin = CRSF_TX_PIN,
    .rxPin = CRSF_RX_PIN,
    .inverted = false,
    .frameRate = 50
};

static const ProtocolConfig defaultSBUS = {
    .enabled = false,
    .baudrate = 100000,
    .txPin = SBUS_TX_PIN,
    .rxPin = SBUS_RX_PIN,
    .inverted = true,
    .frameRate = 50
};

static const ProtocolConfig defaultPPM = {
    .enabled = false,
    .baudrate = 0,
    .txPin = PPM_OUT_PIN,
    .rxPin = 0xFF,
    .inverted = false,
    .frameRate = 50
};

// ============ CONSTRUCTOR ============
ConfigManager::ConfigManager() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        memcpy(&channels[i], &defaultChannels[i], sizeof(ChannelConfig));
    }
    memcpy(&crsfConfig, &defaultCRSF, sizeof(ProtocolConfig));
    memcpy(&sbusConfig, &defaultSBUS, sizeof(ProtocolConfig));
    memcpy(&ppmConfig, &defaultPPM, sizeof(ProtocolConfig));
    loadDefaultSystemConfig();
    
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        calibration.min[i] = 0;
        calibration.max[i] = 255;
        calibration.center[i] = 128;
    }
    calibration.calibrated = false;
}

// ============ INITIALIZATION ============
bool ConfigManager::begin() {
    DEBUG_PRINTLN("[Config] Initializing...");
    if (!prefs.begin("rc_joystick", false)) {
        DEBUG_PRINTLN("[Config] Failed to open preferences!");
        return false;
    }
    
    if (!loadConfig()) {
        DEBUG_PRINTLN("[Config] No saved config, loading defaults...");
        resetToDefaults();
    }
    
    DEBUG_PRINTLN("[Config] Initialization complete");
    return true;
}

// ============ LOAD / SAVE ============
bool ConfigManager::loadConfig() {
    DEBUG_PRINTLN("[Config] Loading configuration...");
    
    if (prefs.isKey("sys_version")) {
        uint8_t version = prefs.getUChar("sys_version", 0);
        if (version != 1) {
            DEBUG_PRINTF("[Config] Config version mismatch (%d vs 1), using defaults\n", version);
            return false;
        }
    } else {
        return false;
    }
    
    prefs.getString("device_name", systemConfig.deviceName, sizeof(systemConfig.deviceName));
    prefs.getString("wifi_ssid", systemConfig.wifiSSID, sizeof(systemConfig.wifiSSID));
    prefs.getString("wifi_pass", systemConfig.wifiPassword, sizeof(systemConfig.wifiPassword));
    systemConfig.apMode = prefs.getBool("ap_mode", true);
    systemConfig.activeProtocol = prefs.getUChar("active_proto", PROTOCOL_CRSF);
    systemConfig.channelCount = prefs.getUChar("ch_count", DEFAULT_CHANNELS);
    systemConfig.debugEnabled = prefs.getBool("debug", false);
    systemConfig.ledEnabled = prefs.getBool("led_en", true);
    systemConfig.statusLED = prefs.getUChar("status_led", LED_STATUS_PIN);
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        loadChannelConfig(i);
    }
    
    crsfConfig.enabled = prefs.getBool("crsf_en", true);
    crsfConfig.baudrate = prefs.getULong("crsf_baud", CRSF_BAUDRATE);
    crsfConfig.frameRate = prefs.getUChar("crsf_fps", 50);
    
    sbusConfig.enabled = prefs.getBool("sbus_en", false);
    sbusConfig.baudrate = prefs.getULong("sbus_baud", SBUS_BAUDRATE);
    sbusConfig.frameRate = prefs.getUChar("sbus_fps", 50);
    
    ppmConfig.enabled = prefs.getBool("ppm_en", false);
    ppmConfig.frameRate = prefs.getUChar("ppm_fps", 50);
    
    calibration.calibrated = prefs.getBool("cal_valid", false);
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        char key[16];
        snprintf(key, sizeof(key), "cal_min_%d", i);
        calibration.min[i] = prefs.getShort(key, 0);
        snprintf(key, sizeof(key), "cal_max_%d", i);
        calibration.max[i] = prefs.getShort(key, 255);
        snprintf(key, sizeof(key), "cal_cen_%d", i);
        calibration.center[i] = prefs.getShort(key, 128);
    }
    
    DEBUG_PRINTLN("[Config] Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfig() {
    DEBUG_PRINTLN("[Config] Saving configuration...");
    
    prefs.putUChar("sys_version", 1);
    prefs.putString("device_name", systemConfig.deviceName);
    prefs.putString("wifi_ssid", systemConfig.wifiSSID);
    prefs.putString("wifi_pass", systemConfig.wifiPassword);
    prefs.putBool("ap_mode", systemConfig.apMode);
    prefs.putUChar("active_proto", systemConfig.activeProtocol);
    prefs.putUChar("ch_count", systemConfig.channelCount);
    prefs.putBool("debug", systemConfig.debugEnabled);
    prefs.putBool("led_en", systemConfig.ledEnabled);
    prefs.putUChar("status_led", systemConfig.statusLED);
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        saveChannelConfig(i);
    }
    
    prefs.putBool("crsf_en", crsfConfig.enabled);
    prefs.putULong("crsf_baud", crsfConfig.baudrate);
    prefs.putUChar("crsf_fps", crsfConfig.frameRate);
    
    prefs.putBool("sbus_en", sbusConfig.enabled);
    prefs.putULong("sbus_baud", sbusConfig.baudrate);
    prefs.putUChar("sbus_fps", sbusConfig.frameRate);
    
    prefs.putBool("ppm_en", ppmConfig.enabled);
    prefs.putUChar("ppm_fps", ppmConfig.frameRate);
    
    prefs.putBool("cal_valid", calibration.calibrated);
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        char key[16];
        snprintf(key, sizeof(key), "cal_min_%d", i);
        prefs.putShort(key, calibration.min[i]);
        snprintf(key, sizeof(key), "cal_max_%d", i);
        prefs.putShort(key, calibration.max[i]);
        snprintf(key, sizeof(key), "cal_cen_%d", i);
        prefs.putShort(key, calibration.center[i]);
    }
    
    prefs.end();
    prefs.begin("rc_joystick", false);
    
    DEBUG_PRINTLN("[Config] Configuration saved successfully");
    return true;
}

bool ConfigManager::resetToDefaults() {
    DEBUG_PRINTLN("[Config] Resetting to defaults...");
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        memcpy(&channels[i], &defaultChannels[i], sizeof(ChannelConfig));
    }
    memcpy(&crsfConfig, &defaultCRSF, sizeof(ProtocolConfig));
    memcpy(&sbusConfig, &defaultSBUS, sizeof(ProtocolConfig));
    memcpy(&ppmConfig, &defaultPPM, sizeof(ProtocolConfig));
    loadDefaultSystemConfig();
    
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        calibration.min[i] = 0;
        calibration.max[i] = 255;
        calibration.center[i] = 128;
    }
    calibration.calibrated = false;
    
    return saveConfig();
}

// ============ CHANNEL OPERATIONS ============
bool ConfigManager::loadChannelConfig(uint8_t ch) {
    if (ch >= MAX_CHANNELS) return false;
    
    char key[32];
    snprintf(key, sizeof(key), "ch%d_src", ch);
    channels[ch].inputSource = prefs.getUChar(key, defaultChannels[ch].inputSource);
    
    snprintf(key, sizeof(key), "ch%d_idx", ch);
    channels[ch].inputIndex = prefs.getChar(key, defaultChannels[ch].inputIndex);
    
    snprintf(key, sizeof(key), "ch%d_rev", ch);
    channels[ch].reversed = prefs.getBool(key, defaultChannels[ch].reversed);
    
    snprintf(key, sizeof(key), "ch%d_min", ch);
    channels[ch].minValue = prefs.getShort(key, defaultChannels[ch].minValue);
    
    snprintf(key, sizeof(key), "ch%d_max", ch);
    channels[ch].maxValue = prefs.getShort(key, defaultChannels[ch].maxValue);
    
    snprintf(key, sizeof(key), "ch%d_cen", ch);
    channels[ch].centerValue = prefs.getShort(key, defaultChannels[ch].centerValue);
    
    snprintf(key, sizeof(key), "ch%d_dz", ch);
    channels[ch].deadzone = prefs.getUChar(key, defaultChannels[ch].deadzone);
    
    snprintf(key, sizeof(key), "ch%d_ex", ch);
    channels[ch].expo = prefs.getUChar(key, defaultChannels[ch].expo);
    
    snprintf(key, sizeof(key), "ch%d_sm", ch);
    channels[ch].smoothing = prefs.getUChar(key, defaultChannels[ch].smoothing);
    
    snprintf(key, sizeof(key), "ch%d_sc", ch);
    channels[ch].scale = prefs.getUShort(key, defaultChannels[ch].scale);
    
    snprintf(key, sizeof(key), "ch%d_en", ch);
    channels[ch].enabled = prefs.getBool(key, defaultChannels[ch].enabled);
    
    return true;
}

bool ConfigManager::saveChannelConfig(uint8_t ch) {
    if (ch >= MAX_CHANNELS) return false;
    
    char key[32];
    snprintf(key, sizeof(key), "ch%d_src", ch);
    prefs.putUChar(key, channels[ch].inputSource);
    
    snprintf(key, sizeof(key), "ch%d_idx", ch);
    prefs.putChar(key, channels[ch].inputIndex);
    
    snprintf(key, sizeof(key), "ch%d_rev", ch);
    prefs.putBool(key, channels[ch].reversed);
    
    snprintf(key, sizeof(key), "ch%d_min", ch);
    prefs.putShort(key, channels[ch].minValue);
    
    snprintf(key, sizeof(key), "ch%d_max", ch);
    prefs.putShort(key, channels[ch].maxValue);
    
    snprintf(key, sizeof(key), "ch%d_cen", ch);
    prefs.putShort(key, channels[ch].centerValue);
    
    snprintf(key, sizeof(key), "ch%d_dz", ch);
    prefs.putUChar(key, channels[ch].deadzone);
    
    snprintf(key, sizeof(key), "ch%d_ex", ch);
    prefs.putUChar(key, channels[ch].expo);
    
    snprintf(key, sizeof(key), "ch%d_sm", ch);
    prefs.putUChar(key, channels[ch].smoothing);
    
    snprintf(key, sizeof(key), "ch%d_sc", ch);
    prefs.putUShort(key, channels[ch].scale);
    
    snprintf(key, sizeof(key), "ch%d_en", ch);
    prefs.putBool(key, channels[ch].enabled);
    
    return true;
}

bool ConfigManager::setChannelConfig(uint8_t ch, ChannelConfig* cfg) {
    if (ch >= MAX_CHANNELS || !cfg) return false;
    memcpy(&channels[ch], cfg, sizeof(ChannelConfig));
    return saveChannelConfig(ch);
}

bool ConfigManager::copyChannelConfig(uint8_t srcCh, uint8_t dstCh) {
    if (srcCh >= MAX_CHANNELS || dstCh >= MAX_CHANNELS) return false;
    memcpy(&channels[dstCh], &channels[srcCh], sizeof(ChannelConfig));
    channels[dstCh].enabled = true;
    return saveChannelConfig(dstCh);
}

bool ConfigManager::resetChannelConfig(uint8_t ch) {
    if (ch >= MAX_CHANNELS) return false;
    memcpy(&channels[ch], &defaultChannels[ch], sizeof(ChannelConfig));
    return saveChannelConfig(ch);
}

void ConfigManager::loadDefaultChannelConfig(uint8_t ch, uint8_t inputIdx) {
    if (ch >= MAX_CHANNELS) return;
    memcpy(&channels[ch], &defaultChannels[ch], sizeof(ChannelConfig));
    channels[ch].inputIndex = inputIdx;
}

void ConfigManager::loadDefaultSystemConfig() {
    strncpy(systemConfig.deviceName, FIRMWARE_NAME, sizeof(systemConfig.deviceName) - 1);
    systemConfig.wifiSSID[0] = '\0';
    systemConfig.wifiPassword[0] = '\0';
    systemConfig.apMode = true;
    systemConfig.activeProtocol = PROTOCOL_CRSF;
    systemConfig.channelCount = DEFAULT_CHANNELS;
    systemConfig.debugEnabled = false;
    systemConfig.ledEnabled = true;
    systemConfig.statusLED = LED_STATUS_PIN;
}

void ConfigManager::loadDefaultProtocols() {
    memcpy(&crsfConfig, &defaultCRSF, sizeof(ProtocolConfig));
    memcpy(&sbusConfig, &defaultSBUS, sizeof(ProtocolConfig));
    memcpy(&ppmConfig, &defaultPPM, sizeof(ProtocolConfig));
}

// ============ PROTOCOL OPERATIONS ============
bool ConfigManager::setProtocolConfig(uint8_t proto, ProtocolConfig* cfg) {
    if (!cfg) return false;
    
    switch(proto) {
        case PROTOCOL_CRSF:
            memcpy(&crsfConfig, cfg, sizeof(ProtocolConfig));
            break;
        case PROTOCOL_SBUS:
            memcpy(&sbusConfig, cfg, sizeof(ProtocolConfig));
            break;
        case PROTOCOL_PPM:
            memcpy(&ppmConfig, cfg, sizeof(ProtocolConfig));
            break;
        default:
            return false;
    }
    return saveConfig();
}

bool ConfigManager::setSystemConfig(SystemConfig* cfg) {
    if (!cfg) return false;
    memcpy(&systemConfig, cfg, sizeof(SystemConfig));
    return saveConfig();
}

// ============ CALIBRATION ============
bool ConfigManager::setCalibration(uint8_t inputIdx, int16_t min, int16_t max, int16_t center) {
    if (inputIdx >= PS_INPUT_COUNT) return false;
    calibration.min[inputIdx] = min;
    calibration.max[inputIdx] = max;
    calibration.center[inputIdx] = center;
    calibration.calibrated = true;
    return saveConfig();
}

// ============ JSON EXPORT / IMPORT ============
String ConfigManager::exportConfig() {
    jsonDoc.clear();
    
    JsonObject sys = jsonDoc.createNestedObject("system");
    sys["deviceName"] = systemConfig.deviceName;
    sys["wifiSSID"] = systemConfig.wifiSSID;
    sys["apMode"] = systemConfig.apMode;
    sys["activeProtocol"] = systemConfig.activeProtocol;
    sys["channelCount"] = systemConfig.channelCount;
    sys["debugEnabled"] = systemConfig.debugEnabled;
    
    JsonArray chArray = jsonDoc.createNestedArray("channels");
    for (int i = 0; i < MAX_CHANNELS; i++) {
        JsonObject ch = chArray.createNestedObject();
        ch["index"] = i;
        ch["inputSource"] = channels[i].inputSource;
        ch["inputIndex"] = channels[i].inputIndex;
        ch["reversed"] = channels[i].reversed;
        ch["minValue"] = channels[i].minValue;
        ch["maxValue"] = channels[i].maxValue;
        ch["centerValue"] = channels[i].centerValue;
        ch["deadzone"] = channels[i].deadzone;
        ch["expo"] = channels[i].expo;
        ch["smoothing"] = channels[i].smoothing;
        ch["scale"] = channels[i].scale;
        ch["enabled"] = channels[i].enabled;
    }
    
    String output;
    serializeJson(jsonDoc, output);
    return output;
}

bool ConfigManager::importConfig(const String& jsonStr) {
    DeserializationError error = deserializeJson(jsonDoc, jsonStr);
    if (error) {
        DEBUG_PRINTF("[Config] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    JsonObjectConst sys = jsonDoc["system"];
    if (!sys.isNull()) {
        strncpy(systemConfig.deviceName, sys["deviceName"] | FIRMWARE_NAME, sizeof(systemConfig.deviceName) - 1);
        strncpy(systemConfig.wifiSSID, sys["wifiSSID"] | "", sizeof(systemConfig.wifiSSID) - 1);
        systemConfig.apMode = sys["apMode"] | true;
        systemConfig.activeProtocol = sys["activeProtocol"] | PROTOCOL_CRSF;
        systemConfig.channelCount = sys["channelCount"] | DEFAULT_CHANNELS;
        systemConfig.debugEnabled = sys["debugEnabled"] | false;
    }
    
    JsonArrayConst chArray = jsonDoc["channels"];
    if (!chArray.isNull()) {
        for (JsonObjectConst ch : chArray) {
            uint8_t idx = ch["index"] | 0;
            if (idx < MAX_CHANNELS) {
                channels[idx].inputSource = ch["inputSource"] | defaultChannels[idx].inputSource;
                channels[idx].inputIndex = ch["inputIndex"] | defaultChannels[idx].inputIndex;
                channels[idx].reversed = ch["reversed"] | defaultChannels[idx].reversed;
                channels[idx].minValue = ch["minValue"] | defaultChannels[idx].minValue;
                channels[idx].maxValue = ch["maxValue"] | defaultChannels[idx].maxValue;
                channels[idx].centerValue = ch["centerValue"] | defaultChannels[idx].centerValue;
                channels[idx].deadzone = ch["deadzone"] | defaultChannels[idx].deadzone;
                channels[idx].expo = ch["expo"] | defaultChannels[idx].expo;
                channels[idx].smoothing = ch["smoothing"] | defaultChannels[idx].smoothing;
                channels[idx].scale = ch["scale"] | defaultChannels[idx].scale;
                channels[idx].enabled = ch["enabled"] | defaultChannels[idx].enabled;
            }
        }
    }
    
    return saveConfig();
}

bool ConfigManager::backupConfig(const String& filename) {
    String json = exportConfig();
    File file = LittleFS.open(filename.c_str(), FILE_WRITE);
    if (!file) return false;
    size_t written = file.print(json);
    file.close();
    return written > 0;
}

bool ConfigManager::restoreConfig(const String& filename) {
    File file = LittleFS.open(filename.c_str(), FILE_READ);
    if (!file) return false;
    
    String content;
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    
    return importConfig(content);
}
