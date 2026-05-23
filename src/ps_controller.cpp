#include "ps_controller.h"

// ============ CONSTRUCTOR ============
PSController::PSController() {
    memset(&state, 0, sizeof(ControllerState));
    memset(&inputState, 0, sizeof(InputState));
    autoReconnect = true;
    lastReconnectAttempt = 0;
    controllerReady = false;
    bp32Initialized = false;
    
    // Default values for analogs
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        inputState.analog[i] = 128;
    }
}

PSController::~PSController() {
    end();
}

// ============ INITIALIZATION ============
bool PSController::begin() {
    DEBUG_PRINTLN("[PS] Initializing Bluepad32...");
    
    // Initialize Bluepad32
    // Note: Bluepad32 setup is handled in main.cpp
    // This is just for state management
    
    bp32Initialized = true;
    state.connected = false;
    state.type = CONTROLLER_TYPE_UNKNOWN;
    
    DEBUG_PRINTLN("[PS] Bluepad32 controller manager initialized");
    return true;
}

void PSController::end() {
    if (state.connected) {
        disconnect();
    }
    bp32Initialized = false;
}

// ============ CONNECTION MANAGEMENT ============
bool PSController::connect() {
    DEBUG_PRINTLN("[PS] Starting controller connection...");
    
    // Bluepad32 handles discovery automatically
    // This function is for manual connection control if needed
    
    return true;
}

void PSController::disconnect() {
    DEBUG_PRINTLN("[PS] Disconnecting controller...");
    
    state.connected = false;
    controllerReady = false;
    
    // Reset input state
    memset(&inputState, 0, sizeof(InputState));
    for (int i = 0; i < PS_INPUT_COUNT; i++) {
        inputState.analog[i] = 128;
    }
}

void PSController::setAutoReconnect(bool enable) {
    autoReconnect = enable;
    DEBUG_PRINTF("[PS] Auto-reconnect: %s\n", autoReconnect ? "enabled" : "disabled");
}

// ============ INTERNAL UPDATE ============
void PSController::update() {
    if (!bp32Initialized) return;
    
    // This will be called from main loop
    // Bluepad32 callbacks will handle actual controller data
    
    // Handle auto-reconnect
    if (autoReconnect && !state.connected) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
            DEBUG_PRINTLN("[PS] Attempting to reconnect...");
            lastReconnectAttempt = now;
            state.disconnectCount++;
            // Re-initiate connection
            connect();
        }
    }
}

void PSController::updateInputState() {
    // Called from Bluepad32 callbacks
    controllerReady = true;
    state.lastUpdate = millis();
}

void PSController::processControllerData() {
    // Additional processing of controller data
    // Can be extended for rumble, LEDs, etc.
}

// ============ INPUT GETTERS ============
int16_t PSController::getAnalog(uint8_t index) {
    if (index < PS_INPUT_COUNT) {
        return inputState.analog[index];
    }
    return 128;
}

bool PSController::getButton(uint8_t index) {
    if (index < PS_INPUT_COUNT) {
        return inputState.button[index];
    }
    return false;
}

// ============ STATUS PRINTS ============
void PSController::printStatus() {
    DEBUG_PRINTLN("=== PS Controller Status ===");
    DEBUG_PRINTF("Connected: %s\n", state.connected ? "YES" : "NO");
    DEBUG_PRINTF("Type: %s\n", 
        state.type == CONTROLLER_TYPE_DS4 ? "DualShock 4" :
        state.type == CONTROLLER_TYPE_DS3 ? "DualShock 3" :
        state.type == CONTROLLER_TYPE_XBOX ? "Xbox" :
        state.type == CONTROLLER_TYPE_SWITCH ? "Switch" : "Unknown");
    DEBUG_PRINTF("Battery: %d%%\n", state.batteryLevel);
    DEBUG_PRINTF("RSSI: %d dBm\n", state.rssi);
    DEBUG_PRINTF("Connected Time: %lu ms\n", getConnectedTime());
    DEBUG_PRINTF("Disconnect Count: %lu\n", state.disconnectCount);
    DEBUG_PRINTLN("============================");
}

void PSController::printControllerData() {
    DEBUG_PRINTLN("=== Controller Data ===");
    DEBUG_PRINTF("Left Stick: X=%d, Y=%d\n", 
        inputState.analog[PS_ANALOG_LX], inputState.analog[PS_ANALOG_LY]);
    DEBUG_PRINTF("Right Stick: X=%d, Y=%d\n", 
        inputState.analog[PS_ANALOG_RX], inputState.analog[PS_ANALOG_RY]);
    DEBUG_PRINTF("L2=%d, R2=%d\n", 
        inputState.analog[PS_ANALOG_L2], inputState.analog[PS_ANALOG_R2]);
    DEBUG_PRINTF("Buttons: Select=%d, Start=%d, PS=%d\n",
        inputState.button[PS_BTN_SELECT], inputState.button[PS_BTN_START],
        inputState.button[PS_BTN_PS]);
    DEBUG_PRINTF("D-Pad: Up=%d, Down=%d, Left=%d, Right=%d\n",
        inputState.button[PS_BTN_DPAD_UP], inputState.button[PS_BTN_DPAD_DOWN],
        inputState.button[PS_BTN_DPAD_LEFT], inputState.button[PS_BTN_DPAD_RIGHT]);
    DEBUG_PRINTF("Shoulders: L1=%d, R1=%d\n",
        inputState.button[PS_BTN_L1], inputState.button[PS_BTN_R1]);
    DEBUG_PRINTF("Face: Triangle=%d, Circle=%d, Cross=%d, Square=%d\n",
        inputState.button[PS_BTN_TRIANGLE], inputState.button[PS_BTN_CIRCLE],
        inputState.button[PS_BTN_CROSS], inputState.button[PS_BTN_SQUARE]);
    DEBUG_PRINTLN("=======================");
}

// ============ BUTTON NAME HELPERS ============
const char* getButtonName(uint8_t buttonIndex) {
    static const char* names[PS_INPUT_COUNT] = {
        "SELECT", "L3", "R3", "START", "DPAD_UP", "DPAD_RIGHT", "DPAD_DOWN", "DPAD_LEFT",
        "L2", "R2", "L1", "R1", "TRIANGLE", "CIRCLE", "CROSS", "SQUARE", "PS",
        "ANALOG_LX", "ANALOG_LY", "ANALOG_RX", "ANALOG_RY", "ANALOG_L2", "ANALOG_R2"
    };
    
    if (buttonIndex < PS_INPUT_COUNT) {
        return names[buttonIndex];
    }
    return "UNKNOWN";
}
