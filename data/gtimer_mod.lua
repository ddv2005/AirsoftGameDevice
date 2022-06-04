require "global"
require "gamebase"

--Game modes
-- 0 Two Timers
-- 1 Two Timers Seq
-- 2 One Timer

gtBtn3Config = {id=const.ID_BTN_3,action=const.ACTION_ON}

function gtWon(self,id)
    sp:playBomb()
    relay:pulse()
    global:setButtonLED(const.ID_BTN_1,false)
    global:setButtonLED(const.ID_BTN_2,false)

    local str = nil
    if((self.m_config.gameMode==0) or (self.m_config.gameMode==3)) then
        if(id==1) then
            str = "Red WON"
        else
            str = "Blue WON"
        end
    else
        str = ""
    end
    screenItems:clear()
    screenItems:drawString(str,const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    screenItems:drawString("Game Over",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    screen:displayScreenItems()
    self.m_gp.m_ap:setActions(emptyActions)
end

function gtWon1(self)
    gtWon(self,1)
end

function gtWon2(self)
    gtWon(self,2)
end

function gtCount1_0(self)
    sp:playOK()
    self:alarmRed()
    global:drawGuage(100,255,0,0)
    global:setButtonLED(const.ID_BTN_1,false)
    global:setButtonLED(const.ID_BTN_2,true)

    local blink = global:getBlinkingInfo();
    blink:clear()
    blink:addStep(10000,250)
    
    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setTimer(self.m_state.timer1,0,blink)

    blink:clear()
    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    segment:setHold(self.m_state.timer2,true,blink)

    local actions = { cActionButtonInput(self.m_btn2Config,gtCount2_0,self), cActionHwInput(gtBtn3Config,gtNeutral0,self), cActionLedTimer({led=1,trigger_time=900},gtWon1,self),cActionLedTimer({led=2,trigger_time=900},gtWon2,self) }
    self.m_gp.m_ap:setActions(actions)
end

function gtCount2_0(self)
    sp:playOK()
    self:alarmBlue()
    global:drawGuage(100,0,0,255)
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,false)

    local blink = global:getBlinkingInfo();
    blink:clear()

    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setHold(self.m_state.timer1,true,blink)

    blink:addStep(10000,250)    
    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    segment:setTimer(self.m_state.timer2,0,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_0,self), cActionHwInput(gtBtn3Config,gtNeutral0,self), cActionLedTimer({led=1,trigger_time=900},gtWon1,self),cActionLedTimer({led=2,trigger_time=900},gtWon2,self) }
    self.m_gp.m_ap:setActions(actions)
end

function gtCount1_1(self)
    sp:playOK()
    self:alarmRed()
    global:drawGuage(100,255,0,0)
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,false)

    local blink = global:getBlinkingInfo();
    blink:clear()
    blink:addStep(10000,250)
    
    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setTimer(self.m_state.timer1,0,blink)

    blink:clear()
    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    segment:setHold(self.m_state.timer2,true,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtNeutral1,self), cActionLedTimer({led=1,trigger_time=900},gtCountEnd1_1,self)}
    self.m_gp.m_ap:setActions(actions)
end

function gtCountEnd1_1(self)
    self.m_state.state = 1
    gtNeutral1(self)
end

function gtCount2_1(self)
    sp:playOK()
    self:alarmBlue()
    global:drawGuage(100,0,0,255)
    global:setButtonLED(const.ID_BTN_1,false)
    global:setButtonLED(const.ID_BTN_2,true)

    local blink = global:getBlinkingInfo();
    blink:clear()

    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setHold(self.m_state.timer1,true,blink)

    blink:addStep(10000,250)    
    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    segment:setTimer(self.m_state.timer2,0,blink)

    local actions = { cActionButtonInput(self.m_btn2Config,gtNeutral1,self), cActionLedTimer({led=2,trigger_time=900},gtWon1,self)}
    self.m_gp.m_ap:setActions(actions)
end

function gtCount1_2(self)
    sp:playOK()
    self:alarmRed()
    global:drawGuage(100,255,0,0)
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,false)

    local blink = global:getBlinkingInfo();
    blink:clear()
    blink:addStep(10000,250)
    
    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setTimer(self.m_state.timer1,0,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtNeutral2,self), cActionLedTimer({led=1,trigger_time=900},gtWon1,self)}
    self.m_gp.m_ap:setActions(actions)
end

function gtCount1_3(self)
    sp:playOK()
    self:alarmRed()
    global:drawGuage(100,255,0,0)
    global:setButtonLED(const.ID_BTN_1,false)
    global:setButtonLED(const.ID_BTN_2,true)

    local blink = global:getBlinkingInfo();
    blink:clear()
    
    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    local stoptime = 3599000+8*3600000;
    if(self.m_config.timer1>10) then
      stoptime = self.m_config.timer1*1000
    end
    blink:addStep(stoptime,250)
    blink:addStep(stoptime-10000,0)
    segment:setStopWatch(self.m_state.timer1,stoptime,blink)

    blink:clear()
    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    segment:setHold(self.m_state.timer2,true,blink)

    local actions = { cActionButtonInput(self.m_btn2Config,gtCount2_3,self), cActionHwInput(gtBtn3Config,gtNeutral3,self), cActionLedCounter({led=1,trigger_time=stoptime},gtWon1,self) }
    self.m_gp.m_ap:setActions(actions)
end

function gtCount2_3(self)
    sp:playOK()
    self:alarmBlue()
    global:drawGuage(100,0,0,255)
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,false)

    local blink = global:getBlinkingInfo();
    blink:clear()

    local segment = global:getSegment1()
    self.m_state.timer1 = segment:getTimer()
    segment:setHold(self.m_state.timer1,true,blink)

    segment = global:getSegment2()
    self.m_state.timer2 = segment:getTimer()
    local stoptime = 3599000+8*3600000;
    if(self.m_config.timer2>10) then
      stoptime = self.m_config.timer2*1000
    end
    blink:addStep(stoptime,250)
    blink:addStep(stoptime-10000,0)
    segment:setStopWatch(self.m_state.timer2,stoptime,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_3,self), cActionHwInput(gtBtn3Config,gtNeutral3,self),cActionLedCounter({led=2,trigger_time=stoptime},gtWon2,self) }
    self.m_gp.m_ap:setActions(actions)
end


function gtNeutral0(self,ft)
    sp:playError()
    global:clearGuage()
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,true)

    local blink = global:getBlinkingInfo();
    blink:clear()
    
    local segment = global:getSegment1()
    if(ft~=true) then
        self.m_state.timer1 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer1,true,blink)

    segment = global:getSegment2()
    if(ft~=true) then
        self.m_state.timer2 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer2,true,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_0,self), cActionButtonInput(self.m_btn2Config,gtCount2_0,self) }
    self.m_gp.m_ap:setActions(actions)
end

function gtNeutral1(self,ft)
    sp:playError()
    global:clearGuage()
    local blink = global:getBlinkingInfo()
    blink:clear()

    local segment = global:getSegment1()
    if(ft~=true) then
        self.m_state.timer1 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer1,true,blink)

    segment = global:getSegment2()
    if(ft~=true) then
        self.m_state.timer2 = segment:getTimer()
    end

    segment:setHold(self.m_state.timer2,true,blink)

    if(self.m_state.state==0) then
        global:setButtonLED(const.ID_BTN_1,true)
        global:setButtonLED(const.ID_BTN_2,false)

        local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_1,self)}
        self.m_gp.m_ap:setActions(actions)
    else
        global:setButtonLED(const.ID_BTN_1,false)
        global:setButtonLED(const.ID_BTN_2,true)

        local actions = { cActionButtonInput(self.m_btn2Config,gtCount2_1,self)}
        self.m_gp.m_ap:setActions(actions)
    end
end

function gtNeutral2(self,ft)
    sp:playError()
    global:clearGuage()
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,false)

    local blink = global:getBlinkingInfo();
    blink:clear()
    
    local segment = global:getSegment1()
    if(ft~=true) then
        self.m_state.timer1 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer1,true,blink)

    local segment = global:getSegment2()
    segment:setNone()

    local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_2,self)}
    self.m_gp.m_ap:setActions(actions)
end

function gtNeutral3(self,ft)
    sp:playError()
    global:clearGuage()
    global:setButtonLED(const.ID_BTN_1,true)
    global:setButtonLED(const.ID_BTN_2,true)

    local blink = global:getBlinkingInfo();
    blink:clear()
    
    local segment = global:getSegment1()
    if(ft~=true) then
        self.m_state.timer1 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer1,true,blink)

    segment = global:getSegment2()
    if(ft~=true) then
        self.m_state.timer2 = segment:getTimer()
    end
    segment:setHold(self.m_state.timer2,true,blink)

    local actions = { cActionButtonInput(self.m_btn1Config,gtCount1_3,self), cActionButtonInput(self.m_btn2Config,gtCount2_3,self) }
    self.m_gp.m_ap:setActions(actions)
end


function gtNeutral(self,ft)
    if(self.m_config.gameMode==0) then
        gtNeutral0(self,ft)
    elseif(self.m_config.gameMode==1) then
        gtNeutral1(self,ft)
    elseif(self.m_config.gameMode==2) then
        gtNeutral2(self,ft)
    elseif(self.m_config.gameMode==3) then
        gtNeutral3(self,ft)
    end
end

function fGameTimer()
    local cGameTimer = cGameBase:extend()

    function cGameTimer:init(config)
        cGameBase.init(self,config)
        self.m_config.gameMode = self.m_config.gameMode or 0
        self.m_config.timer1 = self.m_config.timer1 or 600
        self.m_config.timer2 = self.m_config.timer2 or 600
        self.m_config.useAlarm = self.m_config.useAlarm or false
        self.m_config.longPress = self.m_config.longPress or false
        self.m_config.longPressDuration = self.m_config.longPressDuration or 3000
        self.m_buttonsPressConfig = sButtonConfig(self.m_config.longPress,self.m_config.longPressDuration) 
        self.m_buttonsPressProgress = cButtonScreenProgress()
        self.m_btn1Config = {id=const.ID_BTN_1,button_config=self.m_buttonsPressConfig,progress=self.m_buttonsPressProgress}
        self.m_btn2Config = {id=const.ID_BTN_2,button_config=self.m_buttonsPressConfig,progress=self.m_buttonsPressProgress}
    end

    function cGameTimer:name()
        return "Timer"
    end

    function cGameTimer:alarmRed()
        if(self.m_config.useAlarm==true) then
            relay:pulseCustom(1,100,1)
        end
    end

    function cGameTimer:alarmBlue()
        if(self.m_config.useAlarm==true) then
            relay:pulseCustom(3,100,100)
        end
    end

    function cGameTimer:alarmN()
        if(self.m_config.useAlarm==true) then
            relay:pulseCustom(2,100,100)
        end
    end


    function cGameTimer:startGame(gp)
        cGameBase.startGame(self,gp)
        self.m_state = {}
        self.m_state.state = 0
        if(self.m_config.gameMode==3) then
          self.m_state.timer1 = 0
          self.m_state.timer2 = 0
	    else
          self.m_state.timer1 = self.m_config.timer1*1000
          self.m_state.timer2 = self.m_config.timer2*1000
	    end

        screenItems:clear()
        screenItems:drawString("Press timer",const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        if(self.m_config.longPress==false) then
          screenItems:drawString("button",const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        else
          local str = "button for "..math.floor(self.m_config.longPressDuration/1000).." sec" 
          screenItems:drawString(str,const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
        end
        screen:displayScreenItems()

        gtNeutral(self,true)
    end
    
    function cGameTimer:endGame()
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
    
    return cGameTimer
end

local module = {}

function module.twotimers_func(config)
    config.gameMode = 0
    return fGameTimer()(config)
end

function module.twotimers_setup_func()
    local result = {}
    result[1] = { field="timer1", type = ST_INT, minVal = 10, maxVal = 3599, caption = "RED Timer"}
    result[2] = { field="timer2", type = ST_INT, minVal = 10, maxVal = 3599, caption = "Blue Timer"}
    result[3] = { field="longPress", type = ST_BOOL, caption = "Long Press"}
    result[4] = { field="longPressDuration", type = ST_INT, minVal = 500, maxVal = 100000, caption = "Press Duration"}
    result[5] = { field="useAlarm", type = ST_BOOL, caption = "Use Alarm"}
    return result
end

function module.twotimers_seq_func(config)
    config.gameMode = 1
    return fGameTimer()(config)
end

function module.twotimers_seq_setup_func()
    local result = {}
    result[1] = { field="timer1", type = ST_INT, minVal = 10, maxVal = 3599, caption = "First Timer"}
    result[2] = { field="timer2", type = ST_INT, minVal = 10, maxVal = 3599, caption = "Second Timer"}
    result[3] = { field="longPress", type = ST_BOOL, caption = "Long Press"}
    result[4] = { field="longPressDuration", type = ST_INT, minVal = 500, maxVal = 100000, caption = "Press Duration"}
    result[5] = { field="useAlarm", type = ST_BOOL, caption = "Use Alarm"}
    return result
end

function module.onetimer_func(config)
    config.gameMode = 2
    return fGameTimer()(config)
end

function module.onetimer_setup_func()
    local result = {}
    result[1] = { field="timer1", type = ST_INT, minVal = 10, maxVal = 3599, caption = "Timer"}
    result[2] = { field="longPress", type = ST_BOOL, caption = "Long Press"}
    result[3] = { field="longPressDuration", type = ST_INT, minVal = 500, maxVal = 100000, caption = "Press Duration"}
    result[4] = { field="useAlarm", type = ST_BOOL, caption = "Use Alarm"}
    return result
end

function module.twocounters_func(config)
    config.gameMode = 3
    return fGameTimer()(config)
end

function module.twocounters_setup_func()
    local result = {}
    result[1] = { field="timer1", type = ST_INT, minVal = 0, maxVal = 3599+8*3600, caption = "RED Timer MAX"}
    result[2] = { field="timer2", type = ST_INT, minVal = 0, maxVal = 3599+8*3600, caption = "BLU Timer MAX"}
    result[3] = { field="longPress", type = ST_BOOL, caption = "Long Press"}
    result[4] = { field="longPressDuration", type = ST_INT, minVal = 500, maxVal = 100000, caption = "Press Duration"}
    result[5] = { field="useAlarm", type = ST_BOOL, caption = "Use Alarm"}
    return result
end

return module