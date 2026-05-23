#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>

// ============ COMMON DATA TYPES ============
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef volatile uint32_t vu32;
typedef volatile uint16_t vu16;
typedef volatile uint8_t vu8;
typedef volatile int32_t vs32;

// ============ BOOLEAN ============
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// ============ MIN/MAX ============
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, min, max) (MAX(min, MIN(max, x)))

// ============ BIT OPERATIONS ============
#define BIT(n) (1 << (n))
#define SET_BIT(reg, bit) ((reg) |= (bit))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(bit))
#define READ_BIT(reg, bit) ((reg) & (bit))
#define TOGGLE_BIT(reg, bit) ((reg) ^= (bit))

// ============ ARRAY SIZE ============
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// ============ ATTRIBUTES ============
#define IRAM_ATTR __attribute__((section(".iram1.text")))
#define FLASH_ATTR __attribute__((section(".flash.text")))
#define RTC_DATA_ATTR __attribute__((section(".rtc.data")))

#endif // DATA_TYPES_H
