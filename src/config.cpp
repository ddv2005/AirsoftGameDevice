#include "config.h"

char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
{'1','2','3'},
{'4','5','6'},
{'7','8','9'},
{'*','0','#'}
};

//TBEAM
uint8_t KEYPAD_ROWS_PINS[KEYPAD_ROWS] = {0, 1, 2, 3};
uint8_t KEYPAD_COLS_PINS[KEYPAD_COLS] = {4, 5, 6};
uint8_t neopixels_map[NEOPIXELS_COUNT*NEOPIXELS_PARALLELS] = {
    7,8,
    6,9,
    5,10,
    4,11,
    3,12,
    2,13,
    1,14,
    0,15
    };

SemaphoreHandle_t debugLock = xSemaphoreCreateMutex();