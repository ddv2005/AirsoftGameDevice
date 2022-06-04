#pragma once
#include <Arduino.h>
#include <power.h>
#include <Wire.h>
#include "KeyPadIC2.h"
#include "power.h"
#include "abglobal.h"
#include "abscreen.h"
#include "absegment.h"
#include "concurrency/Thread.h"
#include "esputils.h"
#include "gps/UBloxGPS.h"
#include "radio/LoraRadio.h"
#include "abcommon.h"
#include <sol/sol.hpp>
#include "OneButton\OneButton.h"
#include <NeoPixelBus.h>
#include "neopixels.h"
#include "lua_objects.h"
#include <player.h>
#include "abconfig.h"


class abParams:public cConfig
{
public:    
    bool has_Wire1;
    uint8_t player_rx_pin;
    uint8_t player_tx_pin;
    HardwareSerial *playerSerial;

    uint8_t keypad_rows;
    uint8_t keypad_cols;
    char   *keypad_keys;
    byte   *keypad_pins_rows;
    byte   *keypad_pins_cols;
    TwoWire *keypad_wire;
    uint8_t keypad_address;

    TwoWire *led_wire;
    uint8_t  led1_address;
    uint8_t  led2_address;

    cLoraChannelCondfig channelConfig;

    uint8_t loraAddress;
    uint32_t     loraUpdateTime;

    uint8_t expander_address;

    uint8_t btn1_pin;
    uint8_t btn1_led_pin;

    uint8_t btn2_pin;
    uint8_t btn2_led_pin;

    uint8_t btn3_pin;
    uint8_t switch_pin;
    uint8_t neopixels_pin;
    uint8_t neopixels_count;
    uint8_t neopixels_parallels;
    uint8_t *neopixels_map;
#ifdef USE_I2S    
    uint8_t i2s_pin1;
    uint8_t i2s_pin2;
    uint8_t i2s_pin3;
#endif    

    abParams()
    {
        has_Wire1 = false;
        playerSerial = NULL;
        keypad_keys = NULL;
        keypad_wire = NULL;
        keypad_pins_rows = keypad_pins_cols = NULL;
        loraAddress = 1;
        loraUpdateTime = 10000;
        expander_address = 0;
        neopixels_pin = 0;
        neopixels_count = 0;
        neopixels_parallels = 1;
        neopixels_map = 0;
        initDefaults();
    }

    virtual void initDefaults();
    virtual bool checkConfig();
    virtual void saveConfigValues(JsonDocument &json);
    virtual void loadConfigValues(JsonDocument &json);
};

struct sKeyboardEvent
{
	char kchar;
	int kcode;
	KeyState kstate;
};

struct sLoraPacket
{
    std::vector<byte> data;
};

typedef cProtectedQueue<sKeyboardEvent> cKeyboardEvents;
typedef cProtectedQueue<sLoraPacket> cLoraPackets;

#define ACTION_CLICK        1
#define ACTION_DBLCLICK     2
#define ACTION_LONGCLICK    3
#define ACTION_ON           4
#define ACTION_OFF          5

#define ID_BTN_1       1
#define ID_BTN_2       2
#define ID_BTN_3       3
#define ID_SWITCH_1    4
#define ID_NEOPIXELS   5

#define CMD_ON           1
#define CMD_OFF          2
#define CMD_GUAGE        3
#define CMD_CLEAR        4


struct sAction
{
    uint8_t id;
    uint8_t action;
    uint8_t value;
};

typedef cProtectedQueue<sAction> cActions;

struct sCommand
{
    uint8_t id;
    uint8_t cmd;
    int64_t param1;
    int64_t param2;
    int64_t param3;
};

typedef cProtectedQueue<sCommand> cCommands;

//Player commands
#define PC_STOP 1
#define PC_PLAY 2

struct sPlayerCommand
{
    uint8_t cmd;
    int32_t param1;
    int32_t param2;
};

typedef cProtectedQueue<sPlayerCommand> cPlayerCommands;

#define RA_ON   1
#define RA_OFF  2

struct sRelayAction
{
    uint8_t action;
    int32_t duration;
};

typedef std::vector<sRelayAction> cRelayActions;

class cRelayOutput
{
protected:
    uint8_t m_pin;
    int64_t m_nextActionTime;
    cRelayActions m_actions;
    int32_t m_currentAction;
    bool m_active;
    void reset();
public:
    cRelayOutput(uint8_t pin);
    void begin();
    void tick();
    void pulse(int32_t duration);
    void pulses(int8_t count, int32_t duration_on,int32_t duration_off);
};

class abProcessor:public abGlobal
{
protected:
    typedef concurrency::ThreadFunctionTemplate<abProcessor> abProcessorThreadFunction;
    abParams m_params;
    Screen m_screen;
    bool m_showingBootScreen;
    bool m_has_player;
    cKeyPadI2C *m_keypad;
    abSegment m_led1;
    abSegment m_led2;
    cKeyboardEvents m_keyboardEvents;
    abProcessorThreadFunction *m_i2c2Thread;
    UBloxGPS m_gps;
    cLoraDevice *m_lora;
    abProcessorThreadFunction *m_loraThread;
    cPlayerCommands m_playerCommands;
    abProcessorThreadFunction *m_playerThread;
    cLoraPackets m_loraPackets;
    unsigned long m_lastLoraSend;
    sol::state m_lua;
    PCF8574 *m_expander;
    OneButton *m_btn1;
    OneButton *m_btn2;
    OneButton *m_btn3;
    esp32NeoPixelBus *m_neobus;
    cNeopixels m_neo;
    cActions m_actions;
    cRelayOutput m_relay;

    cLuaScreen m_luaScreen;
    cLuaTimers m_luaTimers;
    sol::function m_luaOnAction;
    sol::function m_luaOnKeypad;
    cBlinkingInfo m_luaBlinkingInfo;
    cCommands m_commands;
    bool m_sw1;
    concurrency::Lock m_neoLock;
    bool m_core0ThreadReady;


    void scanI2Cdevice(TwoWire &wd)
    {
        byte err, addr;
        int nDevices = 0;
        for (addr = 1; addr < 127; addr++)
        {
            wd.beginTransmission(addr);
            err = wd.endTransmission();
            if (err == 0)
            {
                DEBUG_MSG("I2C device found at address 0x%x\n", addr);

                nDevices++;

                if (addr == SSD1306_ADDRESS)
                {
                    m_has_oled = true;
                    DEBUG_MSG("ssd1306 display found\n");
                }
#ifdef AXP192_SLAVE_ADDRESS
                if (addr == AXP192_SLAVE_ADDRESS)
                {
                    m_has_axp192 = true;
                    DEBUG_MSG("axp192 PMU found\n");
                }
#endif
            }
            else if (err == 4)
            {
                DEBUG_MSG("Unknow error at address 0x%x\n", addr);
            }
        }
        if (nDevices == 0)
            DEBUG_MSG("No I2C devices found\n")
        else
            DEBUG_MSG("done\n");
    }

    //Lua API
    void log(const char* msg);
    void logln(const char* msg);
    void setupLua(sol::state &lua);
    int  luaLoadScript(const char* filename);
    sol::object strToLua(const std::string& str);
    std::string luaToStr(const sol::object &obj);
    cLuaScreen &getLuaScreen()
    {
        return m_luaScreen;
    }

    cLuaTimers &getLuaTimers()
    {
        return m_luaTimers;
    }
    void setOnAction(sol::function f)
    {
        m_luaOnAction = f;
    }
    void setOnKeypad(sol::function f)
    {
        m_luaOnKeypad = f;
    }
    void sendCommand(uint8_t id, uint8_t command, int64_t param1=0,int64_t param2=0,int64_t param3=0)
    {
        sCommand cmd;
        cmd.id = id;
        cmd.cmd = command;
        cmd.param1 = param1;
        cmd.param2 = param2;
        cmd.param3 = param3;
        m_commands.push_item(cmd);
    }

    bool getSW1State()
    {
        return m_sw1;
    }

    bool getBtnState(uint8_t id)
    {
        switch(id)
        {
            case ID_BTN_1:
                return m_btn1?m_btn1->getLastState():false;

            case ID_BTN_2:
                return m_btn2?m_btn2->getLastState():false;

            case ID_BTN_3:
                return m_btn3?m_btn3->getLastState():false;

            case ID_SWITCH_1:
                return m_sw1;
        }
        return false;
    }

    bool getBtn1State()
    {
        return m_btn1?m_btn1->getLastState():false;
    }

    bool getBtn2State()
    {
        return m_btn2?m_btn2->getLastState():false;
    }

    bool getBtn3State()
    {
        return m_btn3?m_btn3->getLastState():false;
    }

    void setButtonLED(uint8_t id, bool v)
    {
        sendCommand(id,v?CMD_ON:CMD_OFF);
    }

    void drawGuage(uint8_t value, uint8_t r, uint8_t g, uint8_t b)
    {
        sendCommand(ID_NEOPIXELS, CMD_GUAGE, value,  r+((int64_t)g<<8)+((int64_t)b<<16), 0);
    }

    void drawGuageEx(uint8_t value, uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2)
    {
        sendCommand(ID_NEOPIXELS, CMD_GUAGE, value,  r1+((int64_t)g1<<8)+((int64_t)b1<<16), r2+((int64_t)g2<<8)+((int64_t)b2<<16) );
    }

    void clearGuage()
    {
        sendCommand(ID_NEOPIXELS, CMD_CLEAR);
    }

    abSegment &getSegment1()
    {
        return m_led1;
    }

    abSegment &getSegment2()
    {
        return m_led2;
    }

    void saveLoraConfig();
    cLoraChannelCondfig &getLoraConfig()
    {
        return m_params.channelConfig;
    }

    cBlinkingInfo &getBlinkingInfo()
    {
        return m_luaBlinkingInfo;
    }

    void relayPulse(int32_t duration)
    {
        m_relay.pulse(duration);
    }

    void relayPulses(int8_t count, int32_t duration_on,int32_t duration_off)
    {
        m_relay.pulses(count,duration_on,duration_off);
    }

    void keyboardLoop();
    void mainThread(abProcessorThreadFunction *thread);
    void i2c2Thread(abProcessorThreadFunction *thread);
    void loraThread(abProcessorThreadFunction *thread);
    void playerThread(abProcessorThreadFunction *thread);
    void processKeyEvent(const sKeyboardEvent& event);
    void processAction(const sAction& action);
    void loraSendLocation();
    void processCommand(const sCommand &cmd);

public:
    abProcessor(abParams &params);
    virtual ~abProcessor(){};
    void init();
    void mainInit();
    void mainLoop();
    void playSound(uint8_t folder, uint8_t file);
    void stopSound();
    void onAction(uint8_t id, uint8_t  action, uint8_t value);
};
