require "global"
require "smbase"
require "hwactions"
local sa = require "screenactions"

local mod = {}

function mod.saveMainConfig(o)
    local mainConfig = cConfig("main")    
    mainConfig:save(o)
end

function mod.generateGamesMenu(userData)
    local data = {}
    data.menu = {}
    local i = 1
    for k, v in pairs(userData.games) do
        local gm = {caption = v.caption, name = v.name, key = k, game = v}
        if(v.name==userData.config.lastGame) then
            data.defaultItem = i
        end
        data.menu[i] = gm
        i = i+1
    end
    return data
end

function mod.generateGameParamsMenu(userData)
    local data = {}
    data.menu = {}
    local i = 1
    for k, v in pairs(userData.gc.params) do
        local gp = {caption = v.caption, param = v}
        data.menu[i] = gp
        i = i+1
    end
    return data
end

function mod.onGameParamEditedInt(userData,data)
    userData.gc.config[userData.gameParam.field] = data.val
    userData.gc.configFile:save(userData.gc.config)
    data.selectedItem = userData.selectedGame
    mod.onGameParamsGameMenu(userData,data)
end

function mod.onGameParamEditedBool(userData,data)
    if(data.selectedItem~=nil) then
        userData.gc.config[userData.gameParam.field] = data.selectedItem.val
        userData.gc.configFile:save(userData.gc.config)
    end
    data = {}
    data.selectedItem = userData.selectedGame
    mod.onGameParamsGameMenu(userData,data)
end

function mod.generateBoolMenu(val)
    local result = {}
    result.menu = {}
    result.menu[1] = {caption="Yes",val=true}
    result.menu[2] = {caption="No",val=false}
    if(val==true) then
        result.defaultItem = 1
    else
        result.defaultItem = 2
    end
    return result
end

function mod.onGameParamEdit(userData,data)
    if(data.selectedItem~=nil) then
        userData.gameParam = data.selectedItem.param
        if(data.selectedItem.param.type==ST_INT) then
            local actions = { sa.cActionSInput({caption=userData.gameParam.caption,minVal=userData.gameParam.minVal,maxVal=userData.gameParam.maxVal, val = userData.gc.config[userData.gameParam.field]},mod.onGameParamEditedInt,userData) }
            userData.ap:setActions(actions)
        elseif(data.selectedItem.param.type==ST_CODE) then
            local actions = { sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM,size=userData.gameParam.size,caption=userData.gameParam.caption},mod.onGameParamEditedInt,userData) }
            userData.ap:setActions(actions)
        elseif(data.selectedItem.param.type==ST_BOOL) then
            local data = mod.generateBoolMenu(userData.gc.config[userData.gameParam.field]==true)
            local actions = { sa.cActionSMenu(data,mod.onGameParamEditedBool,userData) }
            userData.ap:setActions(actions)
        else
            data.selectedItem = userData.selectedGame
            mod.onGameParamsGameMenu(userData,data)
        end
    end
end

function mod.onGameParamsGameMenu(userData,data)
    userData.gameParam = nil
    userData.selectedGame = nil
    if(data.selectedItem~=nil) then
        userData.selectedGame = data.selectedItem
        local gc = userData.gp:getParamsByConfig(data.selectedItem.game)
        if(gc~=nil) then
            userData.gc = gc
            local data = mod.generateGameParamsMenu(userData)
            data.caption = "Choice Param"
            data.menu[#data.menu+1] = { caption = "BACK", action = mod.onGameParamsMenu}
            local actions = { sa.cActionSMenu(data,mod.onGameParamEdit,userData) }
            userData.ap:setActions(actions)            
        else
            mod.onGameParamsMenu(userData,data)
        end
    else
        mod.onSetupMenu(userData)
    end
end

function mod.onGameParamsMenu(userData,data)
    userData.gc = nil
    userData.selectedGame = nil
    local data = mod.generateGamesMenu(userData)
    data.caption = "Choice Game"
    data.menu[#data.menu+1] = { caption = "BACK", action = mod.onSetupMenu}
    local actions = { sa.cActionSMenu(data,mod.onGameParamsGameMenu,userData) }
    userData.ap:setActions(actions)
end

function mod.checkConfirmAdminPin(userData, data)
    if(userData.newPin==data.val) then
        userData.config.adminPin = data.val
        mod.saveMainConfig(userData.config)
        mod.onSetupMenu(userData,data)
    else
        sp:playError()
        mod.onChangePinMenu(userData,data)
    end
end

function mod.confirmAdminPin(userData, data)
    userData.newPin = data.val
    local actions = { sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Confirm New PIN"},mod.checkConfirmAdminPin,userData) }
    userData.ap:setActions(actions)
end

function mod.onChangePinMenu(userData,data)
    local actions = { sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Enter New PIN"},mod.confirmAdminPin,userData) }
    userData.ap:setActions(actions)
end

function mod.onGameSelectedMenu(userData,data)
    if(data.selectedItem~=nil) then
        userData.config.lastGame = data.selectedItem.game.name
        mod.saveMainConfig(userData.config)
    end
    mod.menuSelectOrSetup(userData)
end

function mod.onSelectGameMenu(userData,data)
    local data = mod.generateGamesMenu(userData)
    data.caption = "Choice Game"
    data.menu[#data.menu+1] = { caption = "BACK", action = mod.onSetupMenu}
    local actions = { sa.cActionSMenu(data,mod.onGameSelectedMenu,userData) }
    userData.ap:setActions(actions)
end

function mod.onParamEditedInt(userData,data)
    userData.config[userData.setupParam.field] = data.val
    mod.saveMainConfig(userData.config)
    mod.onSetupMenu(userData,data)
end

function mod.onParamEdit(userData,data)
    if(data.selectedItem~=nil) then
        if(data.selectedItem.type==ST_INT) then
            userData.setupParam = data.selectedItem
            local actions = { sa.cActionSInput({caption=userData.setupParam.caption,minVal=userData.setupParam.minVal,maxVal=userData.setupParam.maxVal, val = userData.config[userData.setupParam.field]},mod.onParamEditedInt,userData) }
            userData.ap:setActions(actions)
        else
            mod.onSetupMenu(userData,data)
        end
    end
end

function mod.onRelayTest(userData,data)
    relay:setAlarm(userData.config.relayAlarmCount,userData.config.relayAlarmOn,userData.config.relayAlarmOff)
    relay:pulse()
    mod.onSetupMenu(userData,data)
end

function mod.onLoraEditedInt(userData,data)
    global:getLoraConfig()[userData.setupParam.field] = data.val
    global:saveLoraConfig()
    mod.onLoraParamsMenu(userData,data)
end

function mod.onLoraEdit(userData,data)
    if(data.selectedItem~=nil) then
        if(data.selectedItem.type==ST_INT) then
            userData.setupParam = data.selectedItem
            v = global:getLoraConfig()[userData.setupParam.field]
            local actions = { sa.cActionSInput({caption=userData.setupParam.caption,minVal=userData.setupParam.minVal,maxVal=userData.setupParam.maxVal, val = v},mod.onLoraEditedInt,userData) }
            userData.ap:setActions(actions)
        else
            mod.onSetupMenu(userData,data)
        end
    end
end

function mod.onLoraParamsMenu(userData,data)
    userData.setupParam = nil
    userData.gc = nil
    local menuItems = { 
        {caption="Frequency", field="freq", type = ST_INT, minVal = 902, maxVal = 920},
    }
    menuItems[#menuItems+1] = {caption="BACK",action=mod.onSetupMenu}
    local actions = { sa.cActionSMenu({menu=menuItems,caption="Lora Menu"},mod.onLoraEdit,userData) }
    userData.ap:setActions(actions)
end


function mod.onSetupMenu(userData,data)
    userData.setupParam = nil
    userData.gc = nil
    local menuItems = { 
        {caption="Game Settings",action=mod.onGameParamsMenu},
        {caption="Change PIN",action=mod.onChangePinMenu},
        {caption="Startup Timeout", field="startTimeout", type = ST_INT, minVal = 1, maxVal = 600},
        {caption="Relay Test",action=mod.onRelayTest},
        {caption="Relay Device Type", field="relayType", type = ST_INT, minVal = 0, maxVal = 0},
    }
    if(userData.config.relayType==0) then
        menuItems[#menuItems+1] = {caption="Alarm Count", field="relayAlarmCount", type = ST_INT, minVal = 1, maxVal = 10}
        menuItems[#menuItems+1] = {caption="Alarm On", field="relayAlarmOn", type = ST_INT, minVal = 50, maxVal = 10000}
        menuItems[#menuItems+1] = {caption="Alarm Off", field="relayAlarmOff", type = ST_INT, minVal = 50, maxVal = 30000}
    end

    menuItems[#menuItems+1] = {caption="Lora Settings", action=mod.onLoraParamsMenu}
    menuItems[#menuItems+1] = {caption="BACK",action=mod.menuSelectOrSetup}
    local actions = { sa.cActionSMenu({menu=menuItems,caption="Setup Menu"},mod.onParamEdit,userData) }
    userData.ap:setActions(actions)
end

function mod.menuSelectOrSetup(userData)
    local menuItems = { {caption="Select Game",action=mod.onSelectGameMenu},{caption="Setup",action=mod.onSetupMenu},{caption="Launch Game",action=userData.onFinish} }
    local actions = { sa.cActionSMenu({menu=menuItems,caption="Main Menu"},nil,userData) }
    userData.ap:setActions(actions)
end

function mod.checkAdminPin(userData, data)
    if(userData.config.adminPin==data.val) then
        mod.menuSelectOrSetup(userData)
    else
        sp:playError()
        local actions = { sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Enter PIN"},mod.checkAdminPin,userData), cActionTimer({timer=30000},userData.onFinish,nil) }
        userData.ap:setActions(actions)
    end
end

local module = {}
local function setupLaunch(gameProcessor,config, games, onFinish)
    global:logln("Run setup")
    local setupState = {}
    setupState.gp = gameProcessor
    setupState.ap = gameProcessor.m_ap
    setupState.games = games
    setupState.onFinish = onFinish
    setupState.config = config
    setupState.config.adminPin = setupState.config.adminPin or 0

    local actions = { sa.cActionSFSInput({type=sa.cActionSFSInputconst.SIT_NUM_PWD,size=PIN_SIZE,caption="Enter PIN"},mod.checkAdminPin,setupState),
        cActionTimer({timer=30000},onFinish,nil) }
    setupState.ap:setActions(actions)
end

module.setupLaunch = setupLaunch
return module