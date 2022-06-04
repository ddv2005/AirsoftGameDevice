require "global"
require "smbase"

local module = {}

local c = {}
c.SIT_NUM = 1
c.SIT_NUM_PWD = 2

module.cActionSFSInputconst = c
module.cActionSFSInput = cActionBase:extend()

function module.cActionSFSInput:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.val = self.m_data.val or 0
    self.m_data.type = self.m_data.type or module.cActionSFSInputconst.SIT_NUM
    self.m_data.size = self.m_data.size or 4
    self.m_data.caption = self.m_data.caption or ""
    self.m_data.options = self.m_data.options or 0
end

function module.cActionSFSInput:isFinished()
    return self.m_state.position>=self.m_data.size
end

function module.cActionSFSInput:onKeypad(char, code, state)
    if(state==const.KS_RELEASED) then
        if(char=='*') then
            if(self.m_state.position>0) then
                self.m_state.position = self.m_state.position-1
                self.m_state.val = (self.m_state.val - (self.m_state.val % 10))/10
            end
        elseif(char>='0' and char<='9') then
            sp:playClick()
            if(self.m_state.position<self.m_data.size) then
                local n = char-'0'
                self.m_state.val = self.m_state.val*10 + n
                self.m_state.position = self.m_state.position+1
            end
        end
    end
    self:draw()
    return 1
end

function module.cActionSFSInput:clear()
    screenItems:clear()
    screen:displayScreenItems()
end

function module.cActionSFSInput:draw()
    screenItems:clear()
    screenItems:drawString(self.m_data.caption,const.SCREEN_X/2,2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    local data = ""
    for i = 0,self.m_data.size-1,1 
    do 
        if(self.m_state.position<=i) then
            data = data.."-"
        else
            if(self.m_data.type==module.cActionSFSInputconst.SIT_NUM_PWD) then
                data = data.."*"
            else
                local dd = 10^(self.m_state.position-i)
                local n = ((self.m_state.val % dd) - (self.m_state.val % (dd/10)))/(dd/10)
                data = data..math.floor(n)
            end
        end
    end
    screenItems:drawString(data,const.SCREEN_X/2,const.SCREEN_Y-FS_LARGE-FS_SMALL-2,FS_LARGE+FS_MONO,const.TEXT_ALIGN_CENTER)
    screenItems:drawString("* Back",0,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_LEFT)
    screen:displayScreenItems(1)
end    

function module.cActionSFSInput:beginAction()
    local state = {}
    state.position = 0
    state.val = self.m_data.val
    self.m_state = state

    self:draw()
end

function module.cActionSFSInput:onFinished()
    self.m_data.val = self.m_state.val
    cActionBase.onFinished(self)
end

function module.cActionSFSInput:endAction(isAction)
    --self:clear()
end

------------------------------------------------------------

module.cActionSInput = cActionBase:extend()

function module.cActionSInput:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.val = self.m_data.val or 0
    self.m_data.caption = self.m_data.caption or ""
    self.m_data.minVal = self.m_data.minVal or 0
    self.m_data.maxVal = self.m_data.maxVal or 9999
    self.m_data.options = self.m_data.options or 0
end

function module.cActionSInput:isFinished()
    return self.m_state.m_finished
end

function module.cActionSInput:onKeypad(char, code, state)
    if(self.m_state.m_finished==true) then
        return 0
    end
    if(state==const.KS_RELEASED) then
        if(char=='*') then
            if(self.m_state.val~=0) then
                self.m_state.val = (self.m_state.val - (self.m_state.val % 10))/10
            end
        elseif(char=='#') then
            self.m_state.m_finished = self.m_state.val>=self.m_data.minVal and self.m_state.val<=self.m_data.maxVal
            if(self.m_state.m_finished==false) then
                sp:playError()
            end
        elseif(char>='0' and char<='9') then
            sp:playClick()
            local n = char-'0'
            local v = self.m_state.val*10 + n
            if(v<=self.m_data.maxVal) then
                self.m_state.val = v;
            end
        end
    end
    self:draw()
    return 1
end

function module.cActionSInput:clear()
    screenItems:clear()
    screen:displayScreenItems()
end

function module.cActionSInput:draw()
    screenItems:clear()
    screenItems:drawString(self.m_data.caption,const.SCREEN_X/2,2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    local data = math.floor(self.m_state.val)
    screenItems:drawString(data,const.SCREEN_X/2,const.SCREEN_Y-FS_LARGE-FS_SMALL-2,FS_LARGE+FS_MONO,const.TEXT_ALIGN_CENTER)
    screenItems:drawString("* Back",0,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_LEFT)
    screenItems:drawString("# OK",const.SCREEN_X,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_RIGHT)
    screen:displayScreenItems(1)
end    

function module.cActionSInput:beginAction()
    local state = {}
    state.val = self.m_data.val
    state.m_finished = false
    self.m_state = state

    self:draw()
end

function module.cActionSInput:onFinished()
    self.m_data.val = self.m_state.val
    cActionBase.onFinished(self)
end

function module.cActionSInput:endAction(isAction)
    self:clear()
end


------------------------------------------------------------

module.cActionSMenu = cActionBase:extend()

function module.cActionSMenu:init(data, onAction, userData)
    cActionBase.init(self,data, onAction, userData)
    self.m_data.menu = self.m_data.menu or {}
    self.m_data.defaultItem = self.m_data.defaultItem or 0
end

function module.cActionSMenu:isFinished()
    return self.m_state.m_finished
end

function module.cActionSMenu:onKeypad(char, code, state)
    if(self.m_state.m_finished==true) then
        return 0
    end
    if(state==const.KS_RELEASED) then
        if(char=='*') then
            sp:playClick()
            if(self.m_state.selectedItem>1) then
                self.m_state.selectedItem = self.m_state.selectedItem-1
            else
                self.m_state.selectedItem = #self.m_data.menu
            end
        elseif(char=='#') then
            sp:playClick()
            if(self.m_state.selectedItem<#self.m_data.menu) then
                self.m_state.selectedItem = self.m_state.selectedItem+1
            else
                self.m_state.selectedItem = 1
            end
        elseif(char=='0') then
            sp:playClick()
            self.m_state.m_finished = true
        end
    end
    self:draw()
    return 1
end

function module.cActionSMenu:clear()
    screenItems:clear()
    screen:displayScreenItems()
end

function module.cActionSMenu:draw()
    screenItems:clear()
    screenItems:drawString(self.m_data.caption,const.SCREEN_X/2,2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    local str = ""
    local item = self.m_data.menu[self.m_state.selectedItem]
    if(item~=nil) then
        if(self.m_data.defaultItem==self.m_state.selectedItem) then
            str = "> *"..item.caption.." <"
        else
            str = "> "..item.caption.." <"
        end
    end
    screenItems:drawString(str,const.SCREEN_X/2,const.SCREEN_Y-FS_LARGE-FS_SMALL-2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    screenItems:drawString("* Prev",0,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_LEFT)
    screenItems:drawString("0 OK",const.SCREEN_X/2,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_CENTER)
    screenItems:drawString("# Next",const.SCREEN_X,const.SCREEN_Y-FS_SMALL,FS_SMALL,const.TEXT_ALIGN_RIGHT)
    screen:displayScreenItems(1)
end    

function module.cActionSMenu:beginAction()
    local state = {}
    if(self.m_data.defaultItem>0) then
        state.selectedItem = self.m_data.defaultItem
    else
        state.selectedItem = 1
    end
    state.m_finished = false
    self.m_state = state

    self:draw()
end

function module.cActionSMenu:onFinished()
    local item = self.m_data.menu[self.m_state.selectedItem]
    if(item~=nil) then
        self.m_data.selectedItem = item
        if(item.action~=nil) then
            item.action(self.m_userData,self.m_data)
        elseif(self.m_onAction~=nil) then
            self.m_onAction(self.m_userData,self.m_data)
        end
    else
        if(self.m_onAction~=nil) then
            self.m_onAction(self.m_userData,self.m_data)
        end        
    end
end

function module.cActionSMenu:endAction(isAction)
    self:clear()
end

return module