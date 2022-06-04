#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <freertos\FreeRTOS.h>
#include <freertos\semphr.h>

#define DEBUG_PORT Serial // Serial debug port
#ifdef DEBUG_PORT
extern SemaphoreHandle_t debugLock;
#define DEBUG_MSG(...) {xSemaphoreTake(debugLock, portMAX_DELAY); DEBUG_PORT.printf(__VA_ARGS__); xSemaphoreGive(debugLock); }
#else
#define DEBUG_MSG(...)
#endif

//#define DEBUG_USAGE
//#define DEBUG_MEM
//#define DEBUG_BOARD


// Choice Hardware revisions

//AD_V100 - Lora RF95 & WRGB LED
//#define AD_V100

//AD_V101 - Lora SX1262 & BGR LED
#define AD_V101


#define xstr(s) sstr(s)
#define sstr(s) #s

#define APP_WATCHDOG_SECS 45

#define APP_VERSION 1.2

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
extern char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS];

#define GPS_BAUDRATE 9600
#define LORA_SEND_LOCATION_PERIOD 5000

//TBEAM
extern uint8_t KEYPAD_ROWS_PINS[KEYPAD_ROWS];
extern uint8_t KEYPAD_COLS_PINS[KEYPAD_COLS];

#define LED_WIRE Wire1
#define LED1_ADDR 0x70
#define LED2_ADDR 0x71

#define AC_WIRE Wire1
#define AC_ADDRESS 0x68

#define FLIP_SCREEN_VERTICALLY
#define APPFS SPIFFS
#define SCK_GPIO 5
#define MISO_GPIO 19
#define MOSI_GPIO 27
#define NSS_GPIO 18
#define RESET_GPIO 23 // If defined, this pin will be used to reset the LORA radio

#define GPS_SERIAL_NUM 1
#define GPS_RX_PIN 34
#define GPS_TX_PIN 12

#define PLAYER_RX_PIN 13
#define PLAYER_TX_PIN 2

#define PLAYER_SERIAL Serial2

#define RF95_IRQ_GPIO 26
#define PMU_IRQ 35
#define SSD1306_ADDRESS 0x3C

#define I2C_SDA 21
#define I2C_SCL 22

#define I2C2_SDA 4
#define I2C2_SCL 0

#define KEYPAD_ADDRESS  0x20
#define EXPANDER_ADDRESS  0x21

#define BTN2_PIN        0
#define BTN2_LED_PIN    2

#define BTN1_PIN        1
#define BTN1_LED_PIN    3

#define BTN3_PIN        4
#define SWITCH_PIN      5

#define RELAY_PIN       25

#define NEOPIXELS_PIN   14
#define NEOPIXELS_COUNT 8
#define NEOPIXELS_PARALLELS 2 
extern uint8_t neopixels_map[NEOPIXELS_COUNT*NEOPIXELS_PARALLELS];

//#define DFPLAYER_MINI
//#define DFVOICE
#define USE_I2S

#define PLAYER_I2S_PIN1 2
#define PLAYER_I2S_PIN2 13
#define PLAYER_I2S_PIN3 15


#define SX1262_E22

#define SX1262_IRQ  33
#define SX1262_RST  23
#define SX1262_BUSY 32
#define SX1262_MAXPOWER 22

#define LORA_BW 125.0
#define LORA_SF 10
#define LORA_CR 7
#define LORA_FREQ 915.0
#define LORA_PRE 8
#define LORA_CURRENT 140
#define LORA_SYNC 0x31

#ifdef AD_V100
#undef  USE_SX1262
#define NEOPIXELS_WRGB
#define LORA_POWER 17
#endif

#ifdef AD_V101
#define USE_SX1262
#define LORA_POWER SX1262_MAXPOWER
#undef  NEOPIXELS_WRGB
#endif

#define BATTERY_PIN 36
#define MAIN_CONFIG_FILENAME "/config/main.json"

#endif