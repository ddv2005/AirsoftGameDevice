require "global"
require "config"
require "smbase"
require "hwactions"
require "gamebase"

games = {}

games[1] = { name = "twotimers", caption = "2 Timers", module = "gtimer_mod"}
games[2] = { name = "twotimers_seq", caption = "2 Timers Seq", module = "gtimer_mod"}
games[3] = { name = "onetimer", caption = "One Timer", module = "gtimer_mod"}
games[4] = { name = "twocounters", caption = "2 Counters", module = "gtimer_mod"}
games[5] = { name = "bomb", caption = "Simple Bomb", module = "gbomb_mod"}

global:logln("LUA Started");

local timers = global:getTimers()
local actionsProcessor = cActionsProcessor()

function onKeypad(char, code, state)
    --global:logln("On Keypad char "..char..", code "..code..", state "..state)
    actionsProcessor:onKeypad(char,code,state)
    actionsProcessor:tick()
    collectgarbage() 
end

function onAction(action, id, value)
    --global:logln("On Action action "..action..", id "..id..", value "..value)
    actionsProcessor:onAction(action, id, value)
    actionsProcessor:tick()    
    collectgarbage()
end

function onTimer()
    local now = global:millis()
    actionsProcessor:onTimer(now)
    actionsProcessor:tick()
    collectgarbage()
end
timers:addTimer(500,onTimer)

global:setOnKeypad(onKeypad)
global:setOnAction(onAction)


function onGCTimer()
    collectgarbage() 
end
timers:addTimer(1000,onGCTimer)

global:drawGuage(0,0,0,0)

function defConfig(config)
    config.adminPin = config.adminPin or 0
    config.startTimeout = config.startTimeout or 2
    config.relayType = config.relayType or 0
    config.relayAlarmCount  = config.relayAlarmCount or 1
    config.relayAlarmOn = config.relayAlarmOn or 2000
    config.relayAlarmOff = config.relayAlarmOff or 2000
end

local mainConfig = cConfig("main")
oConfig = mainConfig:load()
defConfig(oConfig)
if(oConfig.relayType==0) then
    relay:setAlarm(oConfig.relayAlarmCount,oConfig.relayAlarmOn,oConfig.relayAlarmOff)
end

screenItems:clear()
screenItems:drawString("Device Started",const.SCREEN_X/2,const.SCREEN_Y/2,FS_MEDIUM,const.TEXT_ALIGN_CENTER_BOTH)
screen:displayScreenItems(1)

gameProcessor = cGameProcessor(oConfig,actionsProcessor)

function onGameStart(userData,data)
    local gameName = userData.config.lastGame
    local game = nil
    for k, v in pairs(games) do
        if(game==nil) then
            game = v
        end
        if(gameName==v.name) then
            game = v
            break
        end
    end
    if(game~=nil) then
        if(gameProcessor:startGameByConfig(game)==true) then
            global:logln("Game "..game.name.." started")
            collectgarbage()
            return
        end
    end
    
    screenItems:clear()
    screenItems:drawString("NO GAMES",const.SCREEN_X/2,const.SCREEN_Y/2,FS_LARGE,const.TEXT_ALIGN_CENTER_BOTH)
    screen:displayScreenItems()

    actionsProcessor:setActions( { cActionBase({},nil,nil) } )
end

function startActions()
    unrequire("setup")
    unrequire("screenactions")
    collectgarbage()
    local actionsState = { config = oConfig }
    local checkSetup = { cActionHwInput({id=const.ID_BTN_3, action = const.ACTION_ON, options=1},onSetup,actionsState), 
        cActionHwInput({id=const.ID_BTN_3, action = const.ACTION_CLICK},onSetup,actionsState),
        cActionHwInput({id=const.ID_BTN_1, action = const.ACTION_CLICK},onGameStart,actionsState),
        cActionHwInput({id=const.ID_BTN_2, action = const.ACTION_CLICK},onGameStart,actionsState),
        cActionTimerCountdown({timer=oConfig.startTimeout*1000, caption = "Starting in"},onGameStart,actionsState) }
    actionsProcessor:setActions(checkSetup)
end

function onSetup(userData,data)
    local s = require "setup"
    s.setupLaunch(gameProcessor, oConfig, games, startActions)
end

startActions()
--onSetup()
actionsProcessor:tick()

collectgarbage() 