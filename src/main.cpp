#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Keypad.h>
#include "config.h"
#include "global.h"
#include "esp_task_wdt.h"
#include "power.h"
#include "abprocessor.h"
#include <esp_spiffs.h>
#include "fsex/fs_ex.h"
#include <driver/adc.h>

abProcessor *global = NULL;

void setup() {
    //Hardware init
    Serial.begin(115200);

    SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);
    SPI.setFrequency(4000000);

    APPFS.begin(true);

    uint32_t seed = esp_random();
    DEBUG_MSG("Setting random seed %u\n", seed);
    randomSeed(seed); // ESP docs say this is fairly random

    DEBUG_MSG("CPU Clock: %d\n", getCpuFrequencyMhz());
    DEBUG_MSG("Total heap: %d\n", ESP.getHeapSize());
    DEBUG_MSG("Free heap: %d\n", ESP.getFreeHeap());
    DEBUG_MSG("Total PSRAM: %d\n", ESP.getPsramSize());
    DEBUG_MSG("Free PSRAM: %d\n", ESP.getFreePsram());

    auto res = esp_task_wdt_init(APP_WATCHDOG_SECS, true);
    assert(res == ESP_OK);

    res = esp_task_wdt_add(NULL);
    assert(res == ESP_OK);

#ifdef I2C_SDA
    Wire.begin(I2C_SDA, I2C_SCL);
#endif
#ifdef BATTERY_PIN
    adcAttachPin(BATTERY_PIN);
#endif
    esp_vfs_spiffs_conf_t spiffs_1 = {
      .base_path = "/spiffs",
      .partition_label = "spiffs",
      .max_files = 64,
      .format_if_mount_failed = true
    };

    esp_vfs_spiffs_conf_t spiffs_2 = {
      .base_path = "/config",
      .partition_label = "config",
      .max_files = 32,
      .format_if_mount_failed = true
    };  
    SPIFFS.end();
    if(esp_vfs_spiffs_register(&spiffs_1)!=ESP_OK || esp_vfs_spiffs_register(&spiffs_2)!=ESP_OK){
        DEBUG_MSG("An Error has occurred while mounting SPIFFS\n");
    }

    size_t total = 0;
    size_t used = 0;
    if(esp_spiffs_info(spiffs_1.partition_label, &total, &used) == ESP_OK)
    {
        DEBUG_MSG("SPIFFS %s used %d, total %d\n",spiffs_1.partition_label, used, total);
    }
    if(esp_spiffs_info(spiffs_2.partition_label, &total, &used) == ESP_OK)
    {
        DEBUG_MSG("SPIFFS %s used %d, total %d\n",spiffs_2.partition_label, used, total);
    }
    FSEX.begin(spiffs_1.base_path);

    abParams params;
    params.player_rx_pin = PLAYER_RX_PIN;
    params.player_tx_pin = PLAYER_TX_PIN;
    params.playerSerial = &PLAYER_SERIAL;

    params.has_Wire1 = true;
    params.led_wire = &LED_WIRE;
    params.led1_address = LED1_ADDR;
    params.led2_address = LED2_ADDR;

    params.keypad_cols = KEYPAD_COLS;
    params.keypad_rows = KEYPAD_ROWS;
    params.keypad_keys = (char*)KEYPAD_KEYS;
    params.keypad_pins_cols = KEYPAD_COLS_PINS;
    params.keypad_pins_rows = KEYPAD_ROWS_PINS;
    params.keypad_wire = &Wire1;
    params.keypad_address = KEYPAD_ADDRESS;

    params.channelConfig.bw = LORA_BW;
    params.channelConfig.sf = LORA_SF;
    params.channelConfig.cr = LORA_CR;
    params.channelConfig.freq = LORA_FREQ;
    params.channelConfig.power = LORA_POWER;
    params.channelConfig.preambleLength = LORA_PRE;
    params.channelConfig.currentLimit = LORA_CURRENT;

    params.expander_address = EXPANDER_ADDRESS;
    params.btn1_pin = BTN1_PIN;
    params.btn1_led_pin = BTN1_LED_PIN;
    params.btn2_pin = BTN2_PIN;
    params.btn2_led_pin = BTN2_LED_PIN;
    params.btn3_pin = BTN3_PIN;
    params.switch_pin = SWITCH_PIN;
    params.neopixels_pin = NEOPIXELS_PIN;
    params.neopixels_count = NEOPIXELS_COUNT;
    params.neopixels_parallels = NEOPIXELS_PARALLELS;
    params.neopixels_map = (uint8_t*)neopixels_map;

    params.loraUpdateTime = LORA_SEND_LOCATION_PERIOD;

#ifdef USE_I2S
    params.i2s_pin1 = PLAYER_I2S_PIN1;
    params.i2s_pin2 = PLAYER_I2S_PIN2;
    params.i2s_pin3 = PLAYER_I2S_PIN3;
#endif    
    
    params.loadConfig(MAIN_CONFIG_FILENAME);
    global = new abProcessor(params);
    global->init();
}

void loop() 
{
    uint32_t msecstosleep = 50;    
    esp_task_wdt_reset();  
    delay(msecstosleep);
}