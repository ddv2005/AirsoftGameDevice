#include "abprocessor.h"
#include "radio/LoraDevRF95.h"
#include "radio/LoraDevSX1262.h"
#include <CayenneLPP.h>
#include "lua/lua_utils.h"
#include "fsex/fs_ex.h"

#ifdef DEBUG_USAGE
class cCPUUsage
{
protected:
    int64_t m_periodStart;
    int64_t m_idleStart;
    int64_t m_workStart;
    int64_t m_idleTime;
    int64_t m_workTime;
    int64_t m_workMaxTime;
    float   m_workUsage;
    float   m_idleUsage;
    int     m_usageCount;
public:
    cCPUUsage()
    {
        m_workMaxTime = m_periodStart = m_idleStart = m_workStart = m_idleTime = m_workTime = 0;
        m_workUsage = 0;
        m_idleUsage = 0;
        m_usageCount = 0;
    }

    void periodStart()
    {
        m_periodStart = esp_timer_get_time();
        m_idleStart = m_workStart = m_idleTime = m_workTime = 0;
    }

    void idleStart()
    {
        m_idleStart = esp_timer_get_time();
    }

    void idleStop()
    {
        m_idleTime += (esp_timer_get_time()-m_idleStart);
        m_idleStart = 0;
    }

    void workStart()
    {
        m_workStart = esp_timer_get_time();  
    }

    void workStop()
    {
        int64_t workTime = (esp_timer_get_time()-m_workStart);
        m_workTime += workTime;
        if(m_workMaxTime<workTime)
            m_workMaxTime = workTime;
        m_workStart = 0;
    }

    void periodStop(const char *msg)
    {
        int64_t period = esp_timer_get_time() - m_periodStart; 
        if(period>0)
        {
            m_workUsage += 100*m_workTime/period;
            m_idleUsage += 100*m_idleTime/period;
            m_usageCount++;
            if(m_usageCount>100)
            {
                DEBUG_MSG("%s work %0.1f%% (%0.2f ms), idle %0.1f%%\n",msg,m_workUsage/m_usageCount, m_workMaxTime/1000.0, m_idleUsage/m_usageCount);
                m_usageCount = 0;
                m_workUsage = m_idleUsage = 0;
                m_workMaxTime = 0;
            }
        }
    }

};
#endif
abProcessor::abProcessor(abParams &params) : m_params(params), m_screen(*this, SSD1306_ADDRESS, I2C_SDA, I2C_SCL), m_keyboardEvents(10),m_playerCommands(20),m_loraPackets(20),m_actions(30),m_relay(RELAY_PIN),m_luaScreen(&m_screen),m_commands(500)
{
    m_core0ThreadReady = false;
    m_showingBootScreen = true;
    m_has_player = false;
    m_keypad = NULL;
    m_loraThread = NULL;
    m_playerThread = NULL;
    m_expander = NULL;
    m_btn1 = NULL;
    m_btn2 = NULL;
    m_btn3 = NULL;
    m_neobus = NULL;
    m_sw1 = false;
}

void abProcessor::saveLoraConfig()
{
    m_params.saveConfig(MAIN_CONFIG_FILENAME);
}

void abProcessor::mainThread(abProcessorThreadFunction *thread)
{
//    int64_t startT = millis();
    mainInit();
#ifdef DEBUG_USAGE
    cCPUUsage usage;
#endif
    while(true)
    {
#ifdef DEBUG_USAGE
        usage.periodStart();
        usage.workStart();
#endif
/*
if((millis()-startT)>2000)
{
    startT = millis();
    char buf[512];
    vTaskGetRunTimeStats(buf);
    Serial.println(buf);
}
*/
        unsigned long now = millis();
        mainLoop();
#ifdef DEBUG_USAGE
        usage.workStop();
        usage.idleStart();
#endif
        int64_t workTime = ((int64_t)millis())-now;
        if(workTime<0)
            workTime = 0;
        if(workTime<10)
            delay(10-workTime);
        else
            delay(1);

#ifdef DEBUG_USAGE
        usage.idleStop();
        usage.periodStop("Main Loop usage");
#endif
    }
}

void abProcessor::mainLoop()
{
    abGlobal::loop();
    if (m_showingBootScreen && (timing::millis() > 1000))
    {
        m_screen.stopBootScreen();
        m_showingBootScreen = false;
    }
    m_keyboardEvents.lock();
    while (m_keyboardEvents.size() > 0)
    {
        sKeyboardEvent event = m_keyboardEvents.front();
        m_keyboardEvents.pop();
        m_keyboardEvents.unlock();
        processKeyEvent(event);
        m_keyboardEvents.lock();
    }
    m_keyboardEvents.unlock();

    m_actions.lock();
    while (m_actions.size() > 0)
    {
        sAction action = m_actions.front();
        m_actions.pop();
        m_actions.unlock();
        processAction(action);
        m_actions.lock();
    }
    m_actions.unlock();

    m_neoLock.lock();
    m_luaTimers.processTimers();
    m_neoLock.unlock();

    m_gps.loop();

    m_relay.tick();

    auto now = millis();
    if((now-m_lastLoraSend)>=m_params.loraUpdateTime)
    {
        m_lastLoraSend = now;
        loraSendLocation();
    }
}

void abProcessor::processAction(const sAction& action)
{
    DEBUG_MSG("Action on %d, %d, %d\n",action.id, action.action, action.value);
    m_neoLock.lock();
    try{
		if(m_luaOnAction.valid())
		{
			m_luaOnAction(action.action, action.id, action.value);
		}
	}catch(std::exception &e)
	{
		DEBUG_MSG("processAction exception %s\n",e.what());
	}
    m_neoLock.unlock();
}

void abProcessor::processKeyEvent(const sKeyboardEvent &event)
{
    m_neoLock.lock();
    try{
		if(m_luaOnAction.valid())
		{
			m_luaOnKeypad(event.kchar,event.kcode,(int)event.kstate);
		}
	}catch(std::exception &e)
	{
		DEBUG_MSG("processKeyEvent exception %s\n",e.what());
	}
    m_neoLock.unlock();
}

static SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);

void abProcessor::init()
{
    auto mainThread = new abProcessorThreadFunction(*this, &abProcessor::mainThread);
    mainThread->start("MainThread", 8192*3, tskIDLE_PRIORITY+1, 1);
}

extern bool canUsePSRAM;// Defined in lua
void abProcessor::mainInit()
{
    DEBUG_MSG("Main Thread on core %d\n",xPortGetCoreID());
    scanI2Cdevice(Wire);
    abGlobal::init();

    sRFModuleConfig loraModule;
#ifdef USE_SX1262    
    loraModule.cs = NSS_GPIO;
    loraModule.irq = SX1262_IRQ;
    loraModule.rst = SX1262_RST;
    loraModule.busy = SX1262_BUSY;
#else
    loraModule.cs = NSS_GPIO;
    loraModule.irq = RF95_IRQ_GPIO;
    loraModule.rst = RESET_GPIO;
    loraModule.busy = 0;
#endif    
    loraModule.spi = &SPI;
    loraModule.spiSettings = &spiSettings;

    if (m_params.keypad_keys)
    {
        m_keypad = new cKeyPadI2C(m_params.keypad_wire, m_params.keypad_address, makeKeymap(m_params.keypad_keys), m_params.keypad_pins_rows, m_params.keypad_pins_cols, m_params.keypad_rows, m_params.keypad_cols);
    }

    if (m_params.playerSerial)
    {
        m_playerThread = new abProcessorThreadFunction(*this, &abProcessor::playerThread);
        m_playerThread->start("Player", 8192, tskIDLE_PRIORITY+1, 0);

        m_has_player = true;
    }
    m_screen.setup();

    m_relay.begin();
    m_i2c2Thread = new abProcessorThreadFunction(*this, &abProcessor::i2c2Thread);
    m_i2c2Thread->start("I2C2", 4096, tskIDLE_PRIORITY+10, 0);

    m_lastLoraSend = millis();
#ifdef USE_SX1262
    m_lora = new cLoraDeviceSX1262(loraModule, m_params.channelConfig);
    int16_t status = m_lora->applyConfig();
    DEBUG_MSG("Lora SX1262 device status %d\n", status);
#else
    m_lora = new cLoraDeviceRF95(loraModule, m_params.channelConfig);
    int16_t status = m_lora->applyConfig();
    DEBUG_MSG("Lora RF95 device status %d\n", status);
#endif
    if(status==0)
    {
        m_loraThread = new abProcessorThreadFunction(*this, &abProcessor::loraThread);
        m_loraThread->start("Lora", 2048, tskIDLE_PRIORITY+1, 1);
    }

    while(!m_core0ThreadReady)
    {
        delay(1);
    }

    delay(100);
    
    if(!m_gps.setup())
    {
        for(int i=0; i<5; i++)
        {
            DEBUG_MSG("Restart GPS\n");
            m_power->gpsOff();
            delay(500);
            m_power->gpsOn();
            delay(1000);
            if(m_gps.setup())
                break;
        }
    }
    
    m_gps.startLock();
    m_gpsStatus->observe(&m_gps.newStatus);
    
    DEBUG_MSG("Setup completed\n");
    m_neoLock.lock();
    canUsePSRAM = false;
    setupLua(m_lua);
    canUsePSRAM = true;
    if(FSEX.exists("/spiffs/main.luac"))
        luaLoadScript("/spiffs/main.luac");
    else
        luaLoadScript("/spiffs/main.lua");
    m_neoLock.unlock();
    DEBUG_MSG("Main stack free %d\n",uxTaskGetStackHighWaterMark(NULL));
    //ble.begin("Airsoft Game Device Updater");
}

void abProcessor::playSound(uint8_t folder, uint8_t file)
{
    if (m_has_player)
    {
        sPlayerCommand cmd;
        cmd.cmd = PC_PLAY;
        cmd.param1 = folder;
        cmd.param2 = file;
        m_playerCommands.push_item(cmd);
    }
}

void abProcessor::stopSound()
{
    if (m_has_player)
    {
        sPlayerCommand cmd;
        cmd.cmd = PC_STOP;
        m_playerCommands.push_item(cmd);
    }
}

#define DegD(i) (i * 1e-7)

void abProcessor::loraSendLocation()
{
    DEBUG_MSG("Lora send location\n");
    CayenneLPP lpp(160);
    lpp.addDigitalInput(LSID_HASLOCK,m_gpsStatus->getHasLock());
    lpp.addDigitalInput(LSID_NSATS,m_gpsStatus->getNumSatellites());
    lpp.addGPS(LSID_GPS,DegD(m_gpsStatus->getLatitude()),DegD(m_gpsStatus->getLongitude()),DegD(m_gpsStatus->getAltitude()));
    lpp.addGenericSensor(LSID_TIMER1,m_led1.getTimer()/1000);
    lpp.addGenericSensor(LSID_TIMER2,m_led2.getTimer()/1000);
    lpp.addDigitalInput(LSID_SWITCH,m_sw1);
    lpp.addDigitalInput(LSID_TIMERS_MODE,m_led1.getMode()|(m_led2.getMode()<<4));
    lpp.addDigitalInput(LSID_HACC,std::min((uint16_t)255,(uint16_t)(m_gpsStatus->getHAcc()/100)));
    sLoraPacket packet;
    packet.data.resize(sizeof(sLoraPacketHeader)+lpp.getSize());
    sLoraPacketHeader *header = (sLoraPacketHeader *)&packet.data[0];
    memcpy(&packet.data[sizeof(sLoraPacketHeader)],lpp.getBuffer(),lpp.getSize());
    header->src = m_params.loraAddress;
    header->dst = 0xFF;
    header->tag = LORA_TAG;
    m_loraPackets.push_item(packet);


    m_screen.m_screenItemsLock.lock();
    sLoraPacket packetScreen;
    packetScreen.data.resize(sizeof(sLoraPacketHeader)+m_screen.m_items.getSize());
    header = (sLoraPacketHeader *)&packetScreen.data[0];
    m_screen.m_items.serialize(&packetScreen.data[sizeof(sLoraPacketHeader)]);
    m_screen.m_screenItemsLock.unlock();
    header->src = m_params.loraAddress;
    header->dst = 0xFF;
    header->tag = LORA_SCREEN_TAG;
    m_loraPackets.push_item(packetScreen);
}

void abProcessor::keyboardLoop()
{
    if (m_keypad->getKeys())
    {
        for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
        {
            if (m_keypad->key[i].stateChanged) // Only find keys that have changed state.
            {
                sKeyboardEvent item;
                item.kstate = m_keypad->key[i].kstate;
                item.kcode = m_keypad->key[i].kcode;
                item.kchar = m_keypad->key[i].kchar;
                m_keyboardEvents.push_item(item);
            }
        }
    }
}

struct callbackData
{
    abProcessor *m_owner;
    void        *m_data;
    uint8_t     m_id;
};

void btnClickCallback(void *data)
{
    if(data)
    {
        callbackData *cdata = (callbackData*)data;
        if(cdata->m_owner)
            cdata->m_owner->onAction(cdata->m_id,ACTION_CLICK,0);
    }
}

void btnDblClickCallback(void *data)
{
    if(data)
    {
        callbackData *cdata = (callbackData*)data;
        if(cdata->m_owner)
            cdata->m_owner->onAction(cdata->m_id,ACTION_DBLCLICK,0);
    }
}

void btnLongClickCallback(void *data)
{
    if(data)
    {
        callbackData *cdata = (callbackData*)data;
        if(cdata->m_owner)
            cdata->m_owner->onAction(cdata->m_id,ACTION_LONGCLICK,0);
    }
}

void btnStartCallback(void *data)
{
    if(data)
    {
        callbackData *cdata = (callbackData*)data;
        if(cdata->m_owner)
            cdata->m_owner->onAction(cdata->m_id,ACTION_ON,0);
    }
}


void btnStopCallback(void *data)
{
    if(data)
    {
        callbackData *cdata = (callbackData*)data;
        if(cdata->m_owner)
            cdata->m_owner->onAction(cdata->m_id,ACTION_OFF,0);
    }
}

void abProcessor::onAction(uint8_t id, uint8_t  action, uint8_t value)
{
    sAction aitem;
    aitem.id = id;
    aitem.action = action;
    aitem.value = value;
    m_actions.push(aitem);
}

void abProcessor::i2c2Thread(abProcessorThreadFunction *thread)
{
    DEBUG_MSG("I2C2 Thread on core %d\n",xPortGetCoreID());
    if(m_params.has_Wire1)
    {
        Wire1.begin(I2C2_SDA, I2C2_SCL,100000L);
        scanI2Cdevice(Wire1);
    }

    m_led1.begin(m_params.led_wire, m_params.led1_address);
    if(m_params.led2_address)
        m_led2.begin(m_params.led_wire, m_params.led2_address);
    m_keypad->begin();
    if(m_params.expander_address)
    {
        m_expander = new PCF8574(&Wire1,m_params.expander_address);
    }
    callbackData btn1Data;
    m_btn1 = new OneButton(m_params.btn1_pin,true,true,m_expander);
    btn1Data.m_owner = this;
    btn1Data.m_data = m_btn1;
    btn1Data.m_id = ID_BTN_1;
    m_btn1->attachClick(&btnClickCallback,&btn1Data);
    m_btn1->attachStartPress(&btnStartCallback,&btn1Data);
    m_btn1->attachStopPress(&btnStopCallback,&btn1Data);
    //m_btn1->attachDoubleClick(&btnDblClickCallback,&btn1Data);
    //m_btn1->attachLongPressStart(&btnLongClickCallback,&btn1Data);

    callbackData btn2Data;
    m_btn2 = new OneButton(m_params.btn2_pin,true,true,m_expander);
    btn2Data.m_owner = this;
    btn2Data.m_data = m_btn2;
    btn2Data.m_id = ID_BTN_2;
    m_btn2->attachClick(&btnClickCallback,&btn2Data);
    m_btn2->attachStartPress(&btnStartCallback,&btn2Data);
    m_btn2->attachStopPress(&btnStopCallback,&btn2Data);
    //m_btn2->attachDoubleClick(&btnDblClickCallback,&btn2Data);
    //m_btn2->attachLongPressStart(&btnLongClickCallback,&btn2Data);


    callbackData btn3Data;
    m_btn3 = new OneButton(m_params.btn3_pin,true,true,m_expander);
    btn3Data.m_owner = this;
    btn3Data.m_data = m_btn3;
    btn3Data.m_id = ID_BTN_3;
    m_btn3->attachClick(&btnClickCallback,&btn3Data);
    m_btn3->attachStartPress(&btnStartCallback,&btn3Data);
    m_btn3->attachStopPress(&btnStopCallback,&btn3Data);
    //m_btn3->attachDoubleClick(&btnDblClickCallback,&btn3Data);
    //m_btn3->attachLongPressStart(&btnLongClickCallback,&btn3Data);


    m_expander->pinMode(m_params.switch_pin,INPUT);
    m_expander->pinMode(m_params.btn1_led_pin,OUTPUT,HIGH);
    m_expander->pinMode(m_params.btn2_led_pin,OUTPUT,HIGH);
    if(m_expander)
        m_expander->begin();
    if(m_params.neopixels_pin && m_params.neopixels_count)
    {
        pinMode(m_params.neopixels_pin, OUTPUT);
        m_neobus = new esp32NeoPixelBus(m_params.neopixels_count*m_params.neopixels_parallels,m_params.neopixels_pin);
        m_neobus->Begin();
        delay(1);
        m_neo.begin(m_neobus,m_params.neopixels_count,m_params.neopixels_parallels,m_params.neopixels_map);
        m_neo.clear();
        m_neo.show();
    }
    m_core0ThreadReady = true;
    DEBUG_MSG("I2C2 stack free %d\n",uxTaskGetStackHighWaterMark(NULL));
#ifdef DEBUG_USAGE
    cCPUUsage usage;
#endif
    while (true)
    {
#ifdef DEBUG_USAGE
        usage.periodStart();
        usage.workStart();
#endif
        unsigned long now = millis();
        m_led1.loop(now);
        m_led2.loop(now);
        keyboardLoop();
        m_btn1->tick();
        m_btn2->tick();
        m_btn3->tick();
#ifndef DEBUG_BOARD        
        bool sw1_new = m_expander->digitalRead(m_params.switch_pin,true)==HIGH;
        if(m_sw1!=sw1_new)
        {
            m_sw1 = sw1_new;
            onAction(ID_SWITCH_1, sw1_new?ACTION_ON:ACTION_OFF,0);
        }
#endif        

        m_commands.lock();
        while (m_commands.size() > 0)
        {
            sCommand cmd = m_commands.front();
            m_commands.pop();
            m_commands.unlock();
            processCommand(cmd);
            m_commands.lock();
        }
        m_commands.unlock();

#ifdef DEBUG_BOARD
        xSemaphoreTake(debugLock, portMAX_DELAY);
        while(Serial.available()>0)
        {
            uint8_t c = Serial.read();
            xSemaphoreGive(debugLock);
            switch(c)
            {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '#':
                case '*':
                {
                    sKeyboardEvent item;
                    item.kstate = KeyState::PRESSED;
                    item.kcode = c-'0';
                    item.kchar = c;
                    m_keyboardEvents.push_item(item);
                    item.kstate = KeyState::RELEASED;
                    m_keyboardEvents.push_item(item);
                    item.kstate = KeyState::IDLE;
                    m_keyboardEvents.push_item(item);
                    break;
                }
                case 'a':
                {
                    onAction(ID_BTN_1,ACTION_ON,0);
                    onAction(ID_BTN_1,ACTION_CLICK,0);
                    onAction(ID_BTN_1,ACTION_OFF,0);
                    break;
                }
                case 's':
                {
                    onAction(ID_BTN_2,ACTION_ON,0);
                    onAction(ID_BTN_2,ACTION_CLICK,0);
                    onAction(ID_BTN_2,ACTION_OFF,0);
                    break;
                }
                case 'd':
                {
                    onAction(ID_BTN_3,ACTION_CLICK,0);
                    break;
                }
                case 'A':
                {
                    onAction(ID_BTN_1,ACTION_LONGCLICK,0);
                    break;
                }
                case 'S':
                {
                    onAction(ID_BTN_2,ACTION_LONGCLICK,0);
                    break;
                }
                case 'D':
                {
                    onAction(ID_BTN_3,ACTION_LONGCLICK,0);
                    break;
                }
                case 'x':
                case 'z':
                {
                    m_sw1 = c=='z';
                    onAction(ID_SWITCH_1, m_sw1?ACTION_ON:ACTION_OFF,0);
                    break;
                }

                default:
                    break;
            }
            xSemaphoreTake(debugLock, portMAX_DELAY);
        }
        xSemaphoreGive(debugLock);
#endif        
#ifdef DEBUG_USAGE
        usage.workStop();
        usage.idleStart();
#endif
        int64_t workTime = ((int64_t)millis())-now;
        if(workTime<0)
            workTime = 0;
        if(workTime<25)
            delay(25-workTime);
        else
            delay(1);
#ifdef DEBUG_USAGE
        usage.idleStop();
        usage.periodStop("I2C2 Loop usage");
#endif
    }

    delete m_btn1;
    if(m_expander)
        delete m_expander;
    m_expander = NULL;
    m_btn1 = NULL;
}


void abProcessor::loraThread(abProcessorThreadFunction *thread)
{
    DEBUG_MSG("Lora Thread on core %d\n",xPortGetCoreID());
    while (true)
    {
        m_loraPackets.lock();
        if(m_loraPackets.size())
        {
            sLoraPacket packet = m_loraPackets.front();
            m_loraPackets.pop();
            m_loraPackets.unlock();
            DEBUG_MSG("Lora transmitting %d\n",packet.data.size());
            auto status = m_lora->transmit(&packet.data[0],packet.data.size());
            DEBUG_MSG("Lora transmit %d\n",status);
        }
        else
            m_loraPackets.unlock();
        delay(25);
    }
}

void abProcessor::log(const char* msg)
{
    DEBUG_MSG(msg);
}

void abProcessor::logln(const char* msg)
{
    DEBUG_MSG("%s\n",msg);
}


sol::object abProcessor::strToLua(const std::string& str)
{
    return strJsonToLuaObject(str, m_lua);
}

std::string abProcessor::luaToStr(const sol::object &obj)
{
    return luaObjectToJsonStr(obj);
}

#define LUA_CONST(C)	lua_consts[#C] = C

void abProcessor::setupLua(sol::state &lua)
{
    lua.open_libraries(sol::lib::base,sol::lib::bit32,sol::lib::math,sol::lib::string,sol::lib::table,sol::lib::count,sol::lib::package,sol::lib::io);
    m_luaScreen.setup(lua);
    m_luaTimers.setup(lua);

    auto lcc = lua.new_simple_usertype<cLoraChannelCondfig>("cLoraChannelCondfig",
        "new",sol::no_constructor,
        LUA_FUNC(cLoraChannelCondfig,freq)
    );
    //lcc["freq"] = &cLoraChannelCondfig::freq;

    lua.new_simple_usertype<cBlinkingInfo>("cBlinkingInfo",
        "new",sol::no_constructor,
        LUA_FUNC(cBlinkingInfo,clear),
        LUA_FUNC(cBlinkingInfo,addStep)
    );

    lua.new_simple_usertype<abSegment>("abSegment",
        "new",sol::no_constructor,
        LUA_FUNC(abSegment,setNone),
        LUA_FUNC(abSegment,setTimer),
        LUA_FUNC(abSegment,setStopWatch),
        LUA_FUNC(abSegment,setHold),
        LUA_FUNC(abSegment,getTimer)
    );

    lua.new_simple_usertype<abProcessor>("abProcessor",
        "new",sol::no_constructor,
        LUA_FUNC(abProcessor,log),
        LUA_FUNC(abProcessor,logln),
        LUA_FUNC(abProcessor,strToLua),
        LUA_FUNC(abProcessor,luaToStr),
        LUA_FUNC(abProcessor,setOnAction),
        LUA_FUNC(abProcessor,setOnKeypad),
        LUA_FUNC(abProcessor,playSound),
        LUA_FUNC(abProcessor,stopSound),
        LUA_FUNC(abProcessor,getSW1State),
        LUA_FUNC(abProcessor,getBtn1State),
        LUA_FUNC(abProcessor,getBtn2State),
        LUA_FUNC(abProcessor,getBtn3State),
        LUA_FUNC(abProcessor,setButtonLED),
        LUA_FUNC(abProcessor,drawGuage),
        LUA_FUNC(abProcessor,drawGuageEx),
        LUA_FUNC(abProcessor,clearGuage),
        LUA_FUNC(abProcessor,getSegment1),
        LUA_FUNC(abProcessor,getSegment2),
        LUA_FUNC(abProcessor,getBlinkingInfo),
        LUA_FUNC(abProcessor,getBtnState),
        LUA_FUNC(abProcessor,relayPulse),
        LUA_FUNC(abProcessor,relayPulses),
        LUA_FUNC(abProcessor,getLoraConfig),
        LUA_FUNC(abProcessor,saveLoraConfig),
        "getScreen",&abProcessor::getLuaScreen,
        "getTimers",&abProcessor::getLuaTimers,
        "millis",&millis
        );
    lua["global"] = this;
    const std::string package_path = lua["package"]["path"];
    lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + "/spiffs/?.luac;/spiffs/?.lua";

    sol::table lua_consts = lua.create_table();
	LUA_CONST(SCREEN_X);
    LUA_CONST(SCREEN_Y);

    LUA_CONST(ID_BTN_1);
    LUA_CONST(ID_BTN_2);
    LUA_CONST(ID_BTN_3);
    LUA_CONST(ID_SWITCH_1);
    LUA_CONST(ID_NEOPIXELS);

    LUA_CONST(ACTION_CLICK);
    LUA_CONST(ACTION_DBLCLICK);
    LUA_CONST(ACTION_LONGCLICK);
    LUA_CONST(ACTION_ON);
    LUA_CONST(ACTION_OFF);

    LUA_CONST(CMD_ON);
    LUA_CONST(CMD_OFF);
    LUA_CONST(CMD_GUAGE);
    LUA_CONST(CMD_CLEAR);

    lua_consts["KS_IDLE"] = (int) KeyState::IDLE;
    lua_consts["KS_PRESSED"] = (int) KeyState::PRESSED;
    lua_consts["KS_HOLD"] = (int) KeyState::HOLD;
    lua_consts["KS_RELEASED"] = (int) KeyState::RELEASED;
    
    lua_consts["TEXT_ALIGN_CENTER"] = (int)OLEDDISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_CENTER;
    lua_consts["TEXT_ALIGN_CENTER_BOTH"] = (int)OLEDDISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_CENTER_BOTH;
    lua_consts["TEXT_ALIGN_LEFT"] = (int)OLEDDISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT;
    lua_consts["TEXT_ALIGN_RIGHT"] = (int)OLEDDISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_RIGHT;

	lua["const"] = lua_consts;
}

int  abProcessor::luaLoadScript(const char* fileName)
{
    auto script = m_lua.load_file(fileName);
    if(script.valid())
    {
        sol::protected_function_result script_result = script();
        if(!script_result.valid())
        {
            sol::error err = script_result;
            DEBUG_MSG("Execute error in LUA script %s - %s\n",fileName,err.what());
            return -1;
        }
    }
    else
    {
        sol::error err = script;
        DEBUG_MSG("Error in LUA script %s - %s\n",fileName,err.what());
        return -1;
    }
    return 0;
}

void abProcessor::processCommand(const sCommand &cmd)
{
    switch (cmd.id)
    {
        case ID_BTN_1:
            m_expander->digitalWrite(m_params.btn1_led_pin,cmd.cmd==CMD_ON?LOW:HIGH);
            break;
        case ID_BTN_2:
            m_expander->digitalWrite(m_params.btn2_led_pin,cmd.cmd==CMD_ON?LOW:HIGH);
            break;

        case ID_NEOPIXELS:
            switch(cmd.cmd)
            {
                case CMD_GUAGE:
                    m_neo.drawGuageEx(cmd.param1,RgbColor(cmd.param2&0xFF,(cmd.param2>>8)&0xFF,(cmd.param2>>16)&0xFF),RgbColor(cmd.param3&0xFF,(cmd.param3>>8)&0xFF,(cmd.param3>>16)&0xFF));
                    break;
                default:
                    m_neo.clear();
                    break;
            }
            m_neoLock.lock();
//            delay(1);
            m_neo.show();
//            delay(1);
            m_neoLock.unlock();
            break;
    default:
        break;
    }
}

void abProcessor::playerThread(abProcessorThreadFunction *thread)
{
    DEBUG_MSG("Player Thread on core %d\n",xPortGetCoreID());
    cPlayer player;
#ifdef USE_I2S
    player.begin(m_params.i2s_pin1,m_params.i2s_pin2,m_params.i2s_pin3);
#else
    m_params.playerSerial->begin(9600, SERIAL_8N1, m_params.player_rx_pin, m_params.player_tx_pin);
    player.begin(m_params.playerSerial);
#endif    
    while (true)
    {
        m_playerCommands.lock();
        if(m_playerCommands.size())
        {
            while(m_playerCommands.size()>1)
                m_playerCommands.pop();
            sPlayerCommand cmd = m_playerCommands.front();
            m_playerCommands.pop();
            m_playerCommands.unlock();
            switch (cmd.cmd)
            {
                case PC_STOP:
                {
                    player.stop();
                    break;
                }

                case PC_PLAY:
                {
                    player.play(cmd.param1,cmd.param2);
                    break;
                }
            
            default:
                break;
            }
        }
        else
            m_playerCommands.unlock();

        delay(25);
    }
}


cRelayOutput::cRelayOutput(uint8_t pin):m_pin(pin)
{
    reset();
}

void cRelayOutput::begin()
{
    if(m_pin)
    {
        pinMode(m_pin,OUTPUT);
        digitalWrite(m_pin,LOW);
    }
    m_active = false;
    reset();
}

void cRelayOutput::tick()
{
    if(m_pin==0) return;
    if(m_nextActionTime<=0)
    {
        if(m_actions.size()>m_currentAction)
        {
            m_nextActionTime = millis()+m_actions[m_currentAction].duration;
            switch(m_actions[m_currentAction].action)
            {
                case RA_ON:
                {
                    DEBUG_MSG("R ON\n");
                    m_active = true;
                    digitalWrite(m_pin,HIGH);
                    break;
                }
                case RA_OFF:
                {
                    DEBUG_MSG("R OFF\n");
                    m_active = false;
                    digitalWrite(m_pin,LOW);
                    break;
                }
            }
        }
        else
        if(m_active)
        {
            DEBUG_MSG("R OFF\n");
            m_active = false;
            digitalWrite(m_pin,LOW);
        }
    }

    if(m_nextActionTime>0)
    {
        int64_t now = millis();
        if(now>=m_nextActionTime)
        {
            m_nextActionTime = 0;
            m_currentAction++;
            tick();
        }
    }
}

void cRelayOutput::pulse(int32_t duration)
{
    pulses(1,duration,0);
}

void cRelayOutput::reset()
{
    m_nextActionTime = 0;
    m_currentAction = 0;
    m_actions.clear();
}

void cRelayOutput::pulses(int8_t count, int32_t duration_on,int32_t duration_off)
{
    reset();
    if(count>0)
    {
        DEBUG_MSG("R %d %d %d\n",count,duration_on,duration_off);
        m_actions.resize(count*2);
        for(int8_t i=0; i<count; i++)
        {
            sRelayAction a;
            a.action = RA_ON;
            a.duration = duration_on;
            m_actions[i*2] = a;
            a.action = RA_OFF;
            a.duration = duration_off;
            m_actions[i*2+1] = a;
        }
        tick();
    }
}

void abParams::initDefaults()
{
    channelConfig = cLoraChannelCondfig();
    channelConfig.bw = LORA_BW;
    channelConfig.sf = LORA_SF;
    channelConfig.cr = LORA_CR;
    channelConfig.freq = LORA_FREQ;
    channelConfig.power = LORA_POWER;
    channelConfig.preambleLength = LORA_PRE;
    channelConfig.syncWord = LORA_SYNC;
    channelConfig.currentLimit = LORA_CURRENT;
}

void abParams::saveConfigValues(JsonDocument &json)
{
    json["freq"] = channelConfig.freq;
    json["bw"] = channelConfig.bw;
    json["sf"] = channelConfig.sf;
    json["cr"] = channelConfig.cr;
    json["preambleLength"] = channelConfig.preambleLength;
    json["syncWord"] = channelConfig.syncWord;
}

void abParams::loadConfigValues(JsonDocument &json)
{
    channelConfig.freq = json["freq"];
    channelConfig.bw = json["bw"];
    channelConfig.sf = json["sf"];
    channelConfig.cr = json["cr"];
    channelConfig.preambleLength = json["preambleLength"];
    channelConfig.syncWord = json["syncWord"];
}

bool abParams::checkConfig()
{
    return false;
}
