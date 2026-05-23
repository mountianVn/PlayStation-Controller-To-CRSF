#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>

// ============ FIRMWARE INFO ============
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_NAME "ESP32-RC-Joystick"
#define BUILD_DATE __DATE__

// ============ HARDWARE CONFIG ============
#ifdef ESP32S3
    #define HAS_USB_SERIAL
    #define HAS_PSRAM
    #define USB_SERIAL_CLASS Serial
#else
    #define USB_SERIAL_CLASS Serial
#endif

// ============ CHANNEL CONFIG ============
#define MAX_CHANNELS 24
#define DEFAULT_CHANNELS 16
#define CHANNEL_MIN_US 1000
#define CHANNEL_MID_US 1500
#define CHANNEL_MAX_US 2000
#define CHANNEL_RANGE_US (CHANNEL_MAX_US - CHANNEL_MIN_US)

// ============ BLUETOOTH CONFIG ============
#define BT_DEVICE_NAME "ESP32-RC-Joystick"
#define BT_PIN "1234"
#define RECONNECT_INTERVAL_MS 5000
#define BT_SCAN_TIME_MS 10000

// ============ RC PROTOCOLS ============
#define PROTOCOL_CRSF 0
#define PROTOCOL_SBUS 1
#define PROTOCOL_PPM  2

// ============ CRSF CONFIG ============
#define CRSF_BAUDRATE 115200
#define CRSF_PACKET_SIZE 26
#define CRSF_CHANNEL_COUNT 16
#define CRSF_FRAME_TYPE_RC_CHANNELS_PACKED 0x16

// ============ SBUS CONFIG ============
#define SBUS_BAUDRATE 100000
#define SBUS_STARTBYTE 0x0F
#define SBUS_ENDBYTE 0x00
#define SBUS_DIGITAL_CHANNEL_MIN 173
#define SBUS_DIGITAL_CHANNEL_MAX 1811

// ============ PPM CONFIG ============
#define PPM_PULSE_US 400
#define PPM_FRAME_US 22500
#define PPM_MIN_US 1000
#define PPM_MAX_US 2000

// ============ WEB SERVER CONFIG ============
#define WEB_SERVER_PORT 80
#define WEBSOCKET_PORT 81
#define AP_IP_ADDRESS IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)
#define AP_CHANNEL 6
#define STA_RECONNECT_INTERVAL_MS 10000
#define WIFI_CONNECT_TIMEOUT_MS 15000

// ============ SYSTEM CONFIG ============
#define SYSTEM_TASK_STACK 4096
#define CONTROLLER_TASK_STACK 8192
#define RC_OUTPUT_TASK_STACK 4096
#define WEB_TASK_STACK 16384
#define STATUS_UPDATE_INTERVAL_MS 100
#define WATCHDOG_TIMEOUT_S 30

// ============ DEBUG CONFIG ============
#define DEBUG_ENABLED
#ifdef DEBUG_ENABLED
    #define DEBUG_BEGIN(x) Serial.begin(x)
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_BEGIN(x)
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// ============ GPIO PINS ============
#ifdef ESP32S3
    // UART pins for CRSF/SBUS/PPM
    #define CRSF_TX_PIN 43
    #define CRSF_RX_PIN 44
    #define SBUS_TX_PIN 39
    #define SBUS_RX_PIN 38
    #define PPM_OUT_PIN 10
    // Status LED
    #define LED_STATUS_PIN 2
    #define LED_CONNECTED_PIN 3
    // Buttons
    #define BTN_CONFIG_PIN 0
    #define BTN_RESET_PIN 9
#else
    // Default ESP32 pins
    #define CRSF_TX_PIN 17
    #define CRSF_RX_PIN 16
    #define SBUS_TX_PIN 4
    #define SBUS_RX_PIN 5
    #define PPM_OUT_PIN 18
    // Status LED
    #define LED_STATUS_PIN 2
    #define LED_CONNECTED_PIN 4
    // Buttons
    #define BTN_CONFIG_PIN 0
    #define BTN_RESET_PIN 9
#endif

// ============ INPUT SOURCES ============
#define INPUT_ANALOG 0
#define INPUT_BUTTON_DIGITAL 1
#define INPUT_BUTTON_ANALOG 2
#define INPUT_DPAD 3

// ============ INPUT BUTTONS ============
#define PS_BTN_SELECT     0
#define PS_BTN_L3         1
#define PS_BTN_R3         2
#define PS_BTN_START      3
#define PS_BTN_DPAD_UP    4
#define PS_BTN_DPAD_RIGHT 5
#define PS_BTN_DPAD_DOWN  6
#define PS_BTN_DPAD_LEFT  7
#define PS_BTN_L2         8
#define PS_BTN_R2         9
#define PS_BTN_L1         10
#define PS_BTN_R1         11
#define PS_BTN_TRIANGLE   12
#define PS_BTN_CIRCLE     13
#define PS_BTN_CROSS      14
#define PS_BTN_SQUARE     15
#define PS_BTN_PS          16
#define PS_ANALOG_LX      17
#define PS_ANALOG_LY      18
#define PS_ANALOG_RX      19
#define PS_ANALOG_RY      20
#define PS_ANALOG_L2      21
#define PS_ANALOG_R2      22
#define PS_INPUT_COUNT    23

// ============ ERROR CODES ============
#define ERR_NONE 0
#define ERR_BT_INIT 1
#define ERR_BT_CONN 2
#define ERR_NO_CONTROLLER 3
#define ERR_INVALID_CHANNEL 4
#define ERR_CRSF_INIT 5
#define ERR_SBUS_INIT 6
#define ERR_PPM_INIT 7
#define ERR_WIFI_INIT 8
#define ERR_WEB_INIT 9
#define ERR_CONFIG_LOAD 10
#define ERR_CONFIG_SAVE 11

#endif // DEFINES_H
