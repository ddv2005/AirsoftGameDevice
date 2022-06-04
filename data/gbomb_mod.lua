require "global"
require "gamebase"
local sa = require "screenactions"

gtBtn1Config = {id=const.ID_BTN_1,action=const.ACTION_CLICK}
gtBtn2Config = {id=const.ID_BTN_2,action=const.ACTION_CLICK}
gtBtn3Config = {id=const.ID_BTN_3,action=const.ACTION_CLICK}

function fGameBomb()
    local cGame = cGameBase:extend()

    function cGame:init(config)
        cGameBase.init(self,config)
        self.m_config.gameMode = self.m_config.gameMode or 0
        self.m_config.timer1 = self.m_config.timer1 or 600
        self.m_config.codeArm = self.m_config.codeArm or 0
        self.m_config.codeDisarm = self.m_config.codeDisarm or 0
        self.m_config.useCodeArm = self.m_config.useCodeArm or false
        self.m_config.useCodeDisarm = self.m_config.useCodeDisarm or false
        self.m_config.useAlarm = self.m_config.useAlarm or false
    end

    function cGame:name()
        return "Bomb"
    end

    function cGame:alarmArmed()
        if(self.m_config.useAlarm==true) then
            relay:pulseCustom(1,100,1)
        end
    end

    function cGame:alarmDisarmed()
        if(self.m_config.useAlarm==true) then
            relay:pulseCustom(3,100,100)
        end
    end

    function cGame:startGame(gp)
        cGameBase.startGame(self,gp)
        self.m_state = {}
        self.m_state.state = 0
        self.m_state.timer1 = self.m_config.timer1*1000

        self:stateNeutral(true)
    end

       
    function cGame:stateBomb()
        sp:playBomb()
        relay:pulse()
        global:drawGuage(100,255,0,0)
        global:setButtonLED(const.ID_BTN_1,false)
        global:setButtonLED(const.ID_BTN_2,false)
    
        local blink = global:getBlinkingInfo();
        blink:clear()
    
        local segment = global:getSegment1()
        self.m_state.timer1 = segment:getTimer()
        segment:setHold(self.m_state.timer1,true,blink)

        screenItems:clear()
        screenItems:drawString("BOOM!!!",const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        screenItems:drawString("Game Over",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        screen:displayScreenItems(1)
        self.m_gp.m_ap:setActions(emptyActions)
    end

    function cGame:stateDisarm()
        global:drawGuage(100,255,0,255)
        local actions = { 
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateSetupDisarmed,self),
            cActionLedTimer({led=1,trigger_time=900},self.stateBomb,self),
            cActionTimerCountdown({timer=30000, caption = "Turn Switch OFF"},self.stateArmed,self)
           }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateCheckDisarmCode(data)
        if(self.m_config.codeDisarm==data.val) then
            sp:playOK()
            self:stateDisarm()
        else
            sp:playError()
            self:stateEnterDisarmCode()
        end
    end

    function cGame:stateEnterDisarmCode()
        local actions = { 
            sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Disarm Code"},self.stateCheckDisarmCode,self) ,
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateBomb,self),
            cActionLedTimer({led=1,trigger_time=900},self.stateBomb,self),
            cActionTimer({timer=30000},self.stateArmed,self)
         }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateArmedSwitchOff()
        global:drawGuage(100,255,0,0)
        sp:playError()
        local actions = { 
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_ON},self.stateArmed,self),
            cActionLedTimer({led=1,trigger_time=900},self.stateBomb,self),
            cActionTimerCountdown({timer=10000, caption = "Turn Switch ON"},self.stateBomb,self)
           }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateArmed()
        global:drawGuage(100,128,0,0)
        global:setButtonLED(const.ID_BTN_1,true)
        global:setButtonLED(const.ID_BTN_2,false)
        --local blink = global:getBlinkingInfo();
        --blink:clear()
        --blink:addStep(10000,250)
        
        --local segment = global:getSegment1()
        --self.m_state.timer1 = segment:getTimer()
        --segment:setTimer(self.m_state.timer1,0,blink)

        screenItems:clear()
        screenItems:drawString("ARMED",const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        screenItems:drawString("Press GREEN btn",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        screen:displayScreenItems()

        local f
        if(self.m_config.useCodeDisarm) then
            f = self.stateEnterDisarmCode
        else
            f = self.stateDisarm
        end
    
        local actions = { cActionHwInput(gtBtn1Config,f,self),cActionHwInput(gtBtn3Config,f,self),
            cActionLedTimer({led=1,trigger_time=900},self.stateBomb,self),
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateArmedSwitchOff,self) }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateSetupArmed()
        local blink = global:getBlinkingInfo();
        blink:clear()
        blink:addStep(10000,250)

        sp:playOK()
        self:alarmArmed()

        local segment = global:getSegment1()
        self.m_state.timer1 = self.m_config.timer1*1000
        segment:setTimer(self.m_state.timer1,0,blink)

        blink:clear()
        segment = global:getSegment2()
        self.m_state.timer2 = segment:getTimer()
        segment:setHold(self.m_state.timer2,true,blink)
    
        self:stateArmed()
    end

    function cGame:stateSetupDisarmed()
        local blink = global:getBlinkingInfo();
        blink:clear()
    
        self:alarmDisarmed()
        
        local segment = global:getSegment2()
        segment:setStopWatch(0,3599000,blink)

        self:stateNeutral(false)
    end    

    function cGame:stateArm()
        global:drawGuage(100,255,255,0)
        local actions = { 
            cActionHwInput({id=const.ID_BTN_3, action = const.ACTION_CLICK},self.stateSetupArmed,self),
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateNeutral,self),
            cActionTimerCountdown({timer=30000, caption = "Press GREEN btn"},self.stateNeutral,self)
           }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateCheckArmCode(data)
        if(self.m_config.codeArm==data.val) then
            sp:playOK()
            self:stateArm()
        else
            sp:playError()
            self:stateEnterArmCode()
        end
    end

    function cGame:stateEnterArmCode()
        global:drawGuage(100,0,255,0)
        local actions = { 
            sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Arm Code"},self.stateCheckArmCode,self) ,
            cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateNeutral,self),
            cActionTimer({timer=30000},self.stateNeutral,nil)
         }
        self.m_gp.m_ap:setActions(actions)
    end

    function cGame:stateNeutral(ft)
        if(ft==true) then
            sp:playOK()
        end
        global:drawGuage(100,0,0,0)
        global:setButtonLED(const.ID_BTN_1,false)
        global:setButtonLED(const.ID_BTN_2,false)
    
        local segment = global:getSegment1()
        segment:setNone()

        local sw = global:getSW1State()
        screenItems:clear()
        screenItems:drawString("DISARMED",const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        if(sw==true) then
            screenItems:drawString("Turn Switch OFF",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        else
            screenItems:drawString("Turn Switch ON",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        end
        screen:displayScreenItems(1)

        if(sw==true) then
            local actions = { 
                cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_OFF},self.stateNeutral,self)
            }
            self.m_gp.m_ap:setActions(actions)
        else
            local f
            if(self.m_config.useCodeArm) then
                f = self.stateEnterArmCode
            else
                f = self.stateArm
            end
            local actions = { 
                cActionHwInput({id=const.ID_SWITCH_1,action=const.ACTION_ON},f,self)
            }
            self.m_gp.m_ap:setActions(actions)
        end
    end
    
    function cGame:endGame()
        cGameBase.endGame(self)
        local segment = global:getSegment1()
        segment:setNone()
    
        segment = global:getSegment2()
        segment:setNone()

        screenItems:clear()
        screen:displayScreenItems()

        global:clearGuage()
        global:setButtonLED(const.ID_BTN_1,false)
        global:setButtonLED(const.ID_BTN_2,false)
    end
    
    return cGame
end

local module = {}

function module.bomb_func(config)
    return fGameBomb()(config)
end

function module.bomb_setup_func()
    local result = {}
    result[1] = { field="timer1", type = ST_INT, minVal = 10, maxVal = 3599, caption = "Bomb Timer"}
    result[2] = { field="codeArm", type = ST_CODE, size = PIN_SIZE, caption = "Arm Code"}
    result[3] = { field="useCodeArm", type = ST_BOOL, caption = "Use Arm Code"}
    result[4] = { field="codeDisarm", type = ST_CODE, size = PIN_SIZE, caption = "Disarm Code"}
    result[5] = { field="useCodeDisarm", type = ST_BOOL, caption = "Use Disarm Code"}
    result[6] = { field="useAlarm", type = ST_BOOL, caption = "Use Alarm"}
    return result
end


return module