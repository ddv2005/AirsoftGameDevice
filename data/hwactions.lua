require "global"
require "smbase"

function sButtonConfig(long_press, press_duration)
    local result = {}
    result.long_press = long_press
    result.press_duration = press_duration
    return result
end    

cButtonProgress = class()

function cButtonProgress:init()
end

function cButtonProgress:progress(value)
end

function cButtonProgress:deinit()
end

function cButtonProgress:start()
end

function cButtonProgress:stop()
end

cButtonScreenProgress = cButtonProgress:extend()

function cButtonScreenProgress:init()
    cButtonProgress.init(self)
end

function cButtonScreenProgress:progress(value)
    local h = 20
    screenItems:clear()
    screenItems:drawProgress(5,const.SCREEN_Y/2,const.SCREEN_X-10,h,value)
    screen:displayScreenOverlayItems(1)
end

function cButtonScreenProgress:deinit()
    self:clear()
end

function cButtonScreenProgress:start()
    self:progress(0)
end

function cButtonScreenProgress:clear()
    screenItems:clear()
    screen:displayScreenOverlayItems(1)
end

function cButtonScreenProgress:stop()
    self:clear()
end


cActionButtonInput = cActionBase:extend()

function cActionButtonInput:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.id = self.m_data.id or const.ID_BTN_1
    self.m_data.button_config = self.m_data.button_config or sButtonConfig(false,0)
end

function cActionButtonInput:isFinished()
    return self.m_state.m_finished
end

function cActionButtonInput:onAction(action, id, value)
    if(self.m_state.m_finished==true) then
        return 0
    end
    if(id==self.m_data.id) then
        if(self.m_data.button_config.long_press==false) then
            if(action==const.ACTION_ON) then
                self.m_state.m_finished = true
            end
        else
            if(self.m_state.m_pressState==0) then
                if(action==const.ACTION_ON) then
                    self.m_state.m_pressState = 1
                    self.m_state.m_pressStart = global:millis()
                    if(self.m_data.progress~=nil) then
                        self.m_data.progress:start()
                    end
                end
            elseif(self.m_state.m_pressState==1) then
                local now = global:millis()
                if((now-self.m_state.m_pressStart)>=self.m_data.button_config.press_duration) then
                    self.m_state.m_pressState = 2
                    self.m_state.m_finished = true
                    if(self.m_data.progress~=nil) then
                        self.m_data.progress:progress(100)
                    end
                elseif(action==const.ACTION_OFF) then
                    self.m_state.m_pressState = 0
                    self.m_state.m_pressStart = 0
                    if(self.m_data.progress~=nil) then
                        self.m_data.progress:stop()
                    end
                end
            end
        end
        return 1
    end
    return 0
end

function cActionButtonInput:onTimer(now)
    if(self.m_state.m_finished==true) then
        return
    end
    if(self.m_state.m_pressState==1) then
        if(self.m_data.progress~=nil) then
            if(self.m_data.button_config.press_duration>0) then
                local val = (now-self.m_state.m_pressStart)*100/self.m_data.button_config.press_duration
                if(val>100) then
                    val = 100
                end
                self.m_data.progress:progress(val)
            end
        end
        if((now-self.m_state.m_pressStart)>=self.m_data.button_config.press_duration) then
            self.m_state.m_pressState = 2
            self.m_state.m_finished = true
        end
    end
end

function cActionButtonInput:beginAction()
    local state = {}
    state.m_finished = false
    state.m_pressState = 0
    state.m_pressstart = 0
    self.m_state = state
end

function cActionButtonInput:endAction(isAction)
    if(self.m_data.progress~=nil) then
        self.m_data.progress:deinit()
    end
end


--------------------------------------------------

cActionHwInput = cActionBase:extend()
-- option = 1 for check just on start
function cActionHwInput:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.id = self.m_data.id or const.ID_BTN_1
    self.m_data.action = self.m_data.action or const.ACTION_CLICK
    self.m_data.options = self.m_data.options or 0
end

function cActionHwInput:isFinished()
    return self.m_state.m_finished
end

function cActionHwInput:onAction(action, id, value)
    if(self.m_state.m_finished==true) then
        return 0
    end
    if(id==self.m_data.id) and (action==self.m_data.action) then
        self.m_state.m_finished = true
        return 1
    end
    return 0
end

function cActionHwInput:beginAction()
    local state = {}
    if( self.m_data.options==1) then
        local s = global:getBtnState(self.m_data.id)
        if(self.m_data.action==const.ACTION_ON) then
            state.m_finished = s
        else
            state.m_finished = s==false
        end
    else
        state.m_finished = false
    end
    self.m_state = state
end

-------------------------------------------

cActionTimer = cActionBase:extend()

function cActionTimer:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.timer  = self.m_data.timer or 1000
end

function cActionTimer:isFinished()
    return self.m_state.m_finished
end

function cActionTimer:onTimer(now)
    if(self.m_state.m_finished==false) then
        self.m_state.m_finished = now>=self.m_state.m_endTime
    end
end

function cActionTimer:beginAction()
    local state = {}
    state.m_finished = false
    state.m_endTime = global:millis()+self.m_data.timer
    self.m_state = state
end


-------------------------------------------

cActionLedTimer = cActionBase:extend()

function cActionLedTimer:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.trigger_time  = self.m_data.trigger_time or 0
    self.m_data.led = self.m_data.led or 1
end

function cActionLedTimer:isFinished()
    return self.m_state.m_finished
end

function cActionLedTimer:onTimer(now)
    if(self.m_state.m_finished==false) then
        local segment = nil
        if(self.m_data.led==1) then
            segment = global:getSegment1()
        else
            segment = global:getSegment2()
        end
        local timer = segment:getTimer()
        self.m_state.m_finished = timer<=self.m_data.trigger_time
    end
end

function cActionLedTimer:beginAction()
    local state = {}
    state.m_finished = false
    self.m_state = state
end

-------------------------------------------
-------------------------------------------

cActionLedCounter = cActionBase:extend()

function cActionLedCounter:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.trigger_time  = self.m_data.trigger_time or 1000
    self.m_data.led = self.m_data.led or 1
end

function cActionLedCounter:isFinished()
    return self.m_state.m_finished
end

function cActionLedCounter:onTimer(now)
    if(self.m_state.m_finished==false) then
        local segment = nil
        if(self.m_data.led==1) then
            segment = global:getSegment1()
        else
            segment = global:getSegment2()
        end
        local timer = segment:getTimer()
        self.m_state.m_finished = timer>=self.m_data.trigger_time
    end
end

function cActionLedCounter:beginAction()
    local state = {}
    state.m_finished = false
    self.m_state = state
end

-------------------------------------------

cActionTimerCountdown = cActionBase:extend()

function cActionTimerCountdown:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.timer  = self.m_data.timer or 1000
    self.m_data.caption = self.m_data.caption or ""
end

function cActionTimerCountdown:isFinished()
    return self.m_state.m_finished
end

function cActionTimerCountdown:onTimer(now)
    if(self.m_state.m_finished==false) then
        self.m_state.m_finished = now>=self.m_state.m_endTime

        if(self.m_state.m_finished==false) then
            local time = math.floor((self.m_state.m_endTime-now+999)/1000).." s"
            if(time~=self.m_state.m_time) then
                self.m_state.m_time = time
                screenItems:clear()
                screenItems:drawString(self.m_data.caption,const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
                screenItems:drawString(self.m_state.m_time,const.SCREEN_X/2,const.SCREEN_Y/2+1,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
                screen:displayScreenItems(1)
            end
        end
    end
end

function cActionTimerCountdown:beginAction()
    local state = {}
    state.m_finished = false
    state.m_endTime = global:millis()+self.m_data.timer
    state.m_time = ""
    self.m_state = state
end
