#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include "defines.h"
#include "config_manager.h"
#include "channel_mapper.h"
#include "wifi_manager.h"
#include "ps_controller.h"
#include "crsf_protocol.h"
#include "sbus_protocol.h"
#include "ppm_protocol.h"
#include "web_server.h"
#include "bluepad32_bridge.h"

// ============ GLOBAL INSTANCES ============
ConfigManager config;
ChannelMapper channelMapper(&config);
PSController psController;
CRSFProtocol crsfProtocol(&Serial1);
SBUSProtocol sbusProtocol(&Serial2);
PPMProtocol ppmProtocol;
WebServer webServer;

// ============ TASK HANDLES ============
TaskHandle_t taskControllerHandle = nullptr;
TaskHandle_t taskRCOutputHandle = nullptr;
TaskHandle_t taskWebHandle = nullptr;
TaskHandle_t taskStatusHandle = nullptr;

// ============ GLOBAL FLAGS ============
volatile bool systemRunning = true;
volatile uint32_t loopCounter = 0;
volatile uint32_t lastLoopTime = 0;
volatile uint32_t taskControllerRunTime = 0;
volatile uint32_t taskRCOutputRunTime = 0;
volatile uint32_t taskWebRunTime = 0;

// ============ STATISTICS ============
struct SystemStats {
    uint32_t loopCount;
    uint32_t controllerUpdates;
    uint32_t rcOutputCount;
    uint32_t lastControllerUpdate;
    uint32_t lastRCOutput;
    float loopRate;
} stats = {0};

// ============ FUNCTION DECLARATIONS ============
void setupSystem();
void initBluepad32();
void updateRCOutput();
void blinkStatusLED();
void setupGPIO();
void printSystemInfo();

// ============ SETUP ============
void setup() {
    // Initialize serial
    DEBUG_BEGIN(115200);
    delay(100);
    
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("=================================================");
    DEBUG_PRINTLN("  ESP32-RC-Joystick Controller");
    DEBUG_PRINTLN("  Version: " FIRMWARE_VERSION);
    DEBUG_PRINTLN("=================================================");
    
    // Setup GPIO
    setupGPIO();
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        DEBUG_PRINTLN("[FS] LittleFS mount failed!");
    } else {
        DEBUG_PRINTLN("[FS] LittleFS mounted");
    }
    
    // Load configuration
    if (!config.begin()) {
        DEBUG_PRINTLN("[Config] Init failed, using defaults");
    }
    
    // Initialize channel mapper
    channelMapper.begin();
    
    // Initialize PS controller
    psController.begin();
    
    // Initialize RC protocols
    ProtocolConfig* crsfCfg = config.getProtocolConfig(PROTOCOL_CRSF);
    if (crsfCfg && crsfCfg->enabled) {
        crsfProtocol.begin(crsfCfg->baudrate);
    }
    
    ProtocolConfig* sbusCfg = config.getProtocolConfig(PROTOCOL_SBUS);
    if (sbusCfg && sbusCfg->enabled) {
        sbusProtocol.begin(sbusCfg->baudrate, sbusCfg->inverted);
    }
    
    ProtocolConfig* ppmCfg = config.getProtocolConfig(PROTOCOL_PPM);
    if (ppmCfg && ppmCfg->enabled) {
        ppmProtocol.begin(ppmCfg->txPin, 8, ppmCfg->inverted);
    }
    
    // Initialize WiFi
    SystemConfig* sysCfg = config.getSystemConfig();
    if (sysCfg->wifiSSID[0] != '\0' && sysCfg->wifiPassword[0] != '\0') {
        if (!wifiManager.begin(sysCfg->wifiSSID, sysCfg->wifiPassword)) {
            DEBUG_PRINTLN("[WiFi] STA connection failed, starting AP");
            wifiManager.beginAP(FIRMWARE_NAME);
        }
    } else {
        // Start AP mode
        char apName[64];
        snprintf(apName, sizeof(apName), "%s-%08X", FIRMWARE_NAME, (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF));
        wifiManager.beginAP(apName);
    }
    
    // Initialize Bluepad32
    initBluepad32();
    
    // Initialize web server
    webServer.begin();
    
    // Create FreeRTOS tasks
    setupSystem();
    
    // Print system info
    printSystemInfo();
    
    DEBUG_PRINTLN("[Main] Setup complete!");
    DEBUG_PRINTLN("=================================================");
}

// ============ MAIN LOOP ============
void loop() {
    uint32_t loopStart = micros();
    
    // Update PS controller via Bluepad32
    processControllers(&psController);
    
    // Update channel mapper with controller input
    InputState* inputs = psController.getInputState();
    channelMapper.setAllInputs(*inputs);
    channelMapper.update();
    
    // Update RC output
    updateRCOutput();
    
    // Update web server
    webServer.update();
    
    // WiFi reconnect check
    if (!wifiManager.isConnected() && !wifiManager.isAPMode()) {
        wifiManager.reconnect();
    }
    
    // Status LED blink
    blinkStatusLED();
    
    // Statistics
    loopCounter++;
    stats.loopCount++;
    
    uint32_t loopTime = micros() - loopStart;
    if (loopTime > 0) {
        stats.loopRate = 1000000.0 / loopTime;
    }
    
    // Watchdog reset
    yield();
}

// ============ TASK: STATUS UPDATE ============
void taskStatusUpdate(void* parameter) {
    DEBUG_PRINTLN("[Task] Status update task started");
    
    while (systemRunning) {
        // Print periodic status
        DEBUG_PRINTF("[Status] Heap: %d, LoopRate: %.1fHz\n", 
            ESP.getFreeHeap(), stats.loopRate);
        
        if (psController.isConnected()) {
            DEBUG_PRINTF("[Status] Controller: %s, Battery: %d%%\n",
                psController.getName(), psController.getBatteryLevel());
        } else {
            DEBUG_PRINTLN("[Status] Controller: Disconnected");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    vTaskDelete(NULL);
}

// ============ SETUP SYSTEM (TASKS) ============
void setupSystem() {
    DEBUG_PRINTLN("[Main] Creating FreeRTOS tasks...");
    
    // Task: Status Update
    xTaskCreatePinnedToCore(
        taskStatusUpdate,
        "Status",
        SYSTEM_TASK_STACK,
        NULL,
        1,
        &taskStatusHandle,
        1
    );
    
    DEBUG_PRINTLN("[Main] Tasks created");
}

// ============ BLUEPAD32 INITIALIZATION ============
void initBluepad32() {
    DEBUG_PRINTLN("[BP32] Initializing Bluepad32...");
    
    // Initialize Bluepad32 bridge
    processControllers(nullptr);
    
    DEBUG_PRINTLN("[BP32] Bluepad32 initialized (stub mode)");
}

// ============ UPDATE RC OUTPUT ============
void updateRCOutput() {
    uint16_t* channels = channelMapper.getAllChannels();
    uint8_t chCount = config.getChannelCount();
    
    if (crsfProtocol.isEnabled()) {
        crsfProtocol.sendChannels(channels, chCount);
    }
    
    if (sbusProtocol.isEnabled()) {
        sbusProtocol.sendChannels(channels, chCount);
    }
    
    if (ppmProtocol.isEnabled()) {
        ppmProtocol.sendChannels(channels, chCount);
    }
}

// ============ GPIO SETUP ============
void setupGPIO() {
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);
    
    pinMode(LED_CONNECTED_PIN, OUTPUT);
    digitalWrite(LED_CONNECTED_PIN, LOW);
    
    pinMode(BTN_CONFIG_PIN, INPUT_PULLUP);
    pinMode(BTN_RESET_PIN, INPUT_PULLUP);
    
    DEBUG_PRINTLN("[GPIO] Pins configured");
}

// ============ STATUS LED BLINK ============
void blinkStatusLED() {
    static uint32_t lastBlink = 0;
    static bool ledState = false;
    
    uint32_t now = millis();
    uint32_t interval = psController.isConnected() ? 500 : 1000;
    
    if (now - lastBlink >= interval) {
        ledState = !ledState;
        digitalWrite(LED_STATUS_PIN, ledState ? HIGH : LOW);
        lastBlink = now;
    }
    
    digitalWrite(LED_CONNECTED_PIN, psController.isConnected() ? HIGH : LOW);
}

// ============ PRINT SYSTEM INFO ============
void printSystemInfo() {
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("=================== SYSTEM INFO ===================");
    DEBUG_PRINTF("Chip Model: %s\n", ESP.getChipModel());
    DEBUG_PRINTF("Chip Revision: %d\n", ESP.getChipRevision());
    DEBUG_PRINTF("Chip Cores: %d\n", ESP.getChipCores());
    DEBUG_PRINTF("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    DEBUG_PRINTF("PSRAM Size: %d bytes\n", ESP.getPsramSize());
    DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("Chip ID: %08X\n", (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF));
    DEBUG_PRINTLN("==================================================");
    DEBUG_PRINTF("WiFi Mode: %s\n", wifiManager.isAPMode() ? "AP" : "Station");
    DEBUG_PRINTF("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("Web Server: http://%s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTLN("==================================================");
    DEBUG_PRINTLN();
}
