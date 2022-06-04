require "class"

cActionBase = class()

function cActionBase:init(data, onAction, userData)
    self.m_userData = userData
    self.m_data = data
    self.m_onAction = onAction
end

function cActionBase:beginAction()
end    

function cActionBase:endAction(isAction)
end    

function cActionBase:onTimer(now)
end

function cActionBase:isFinished()
    return false
end

function cActionBase:onFinished()
    if(self.m_onAction~=nil) then
        self.m_onAction(self.m_userData,self.m_data)
    end
end
 
function cActionBase:onKeypad(char, code, state)
    return 0
end

function cActionBase:onAction(action, id, value)
    return 0
end
-------------------------------------------------------

cActionsProxy = class()

function cActionsProxy:init(actions)
    self.m_actions = actions
    self.m_finishedActionKey = nil
end

function cActionsProxy:beginAction()
    for k, a in pairs(self.m_actions) do
        a:beginAction()
    end    
end    

function cActionsProxy:endAction(isAction)
    for k, a in pairs(self.m_actions) do
        if(isAction==false) then
            result = a:endAction(false)
        else
            result = a:endAction(k==self.m_finishedActionKey)
        end
    end    
end    

function cActionsProxy:isFinished()
    local result = false
    for k, a in pairs(self.m_actions) do
        result = a:isFinished()
        if(result==true) then
            self.m_finishedActionKey = k
            a:onFinished()
            break
        end
    end    
    return result
end

function cActionsProxy:onKeypad(char, code, state)
    local result = 0
    for k, a in pairs(self.m_actions) do
        result = a:onKeypad(char, code, state)
        if(result==1) then
            break
        end
    end    
    return result
end

function cActionsProxy:onAction(action, id, value)
    local result = 0
    for k, a in pairs(self.m_actions) do
        result = a:onAction(action, id, value)
        if(result==1) then
            break
        end
    end    
    return result
end

function cActionsProxy:onTimer(now)
    for k, a in pairs(self.m_actions) do
        result = a:onTimer(now)
    end    
end

-------------------------------------------------------
cActionsProcessor = class()

function cActionsProcessor:init()
    self.m_pendingActions = nil
    self.m_proxy = nil
    self.m_isActionFinished = false
end

function cActionsProcessor:setActions(actions)
    self.m_pendingActions = actions
end


function cActionsProcessor:internalSwitchActions()
    if(self.m_pendingActions~=nil) then
        if(self.m_proxy~=nil) then
            self.m_proxy:endAction(self.m_isActionFinished)
        end
        self.m_proxy = cActionsProxy(self.m_pendingActions)
        self.m_pendingActions = nil
        self.m_isActionFinished = false
        self.m_proxy:beginAction()
        return true
    else
        return false
    end
end

function cActionsProcessor:tick()
    if(self:internalSwitchActions()==false) then
        if(self.m_proxy~=nil) then
            self.m_isActionFinished = self.m_isActionFinished or self.m_proxy:isFinished()
            self:internalSwitchActions()
        end
    end
end

function cActionsProcessor:onKeypad(char, code, state)
    self.m_proxy:onKeypad(char, code, state)
end

function cActionsProcessor:onAction(action, id, value)
    self.m_proxy:onAction(action, id, value)
end

function cActionsProcessor:onTimer(now)
    self.m_proxy:onTimer(now)
end

emptyActions = { cActionBase({},nil,nil) } 