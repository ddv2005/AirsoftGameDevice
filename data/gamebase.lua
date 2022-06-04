require "class"
require "config"
require "smbase"


cGameBase = class()

function cGameBase:init(config)
    self.m_config = config or {}
end

function cGameBase:name()
    return "Base"
end

function cGameBase:startGame(gp)
    self.m_gp = gp
end

function cGameBase:endGame()
    self.m_gp = nil
end

----------------------------

cGameProcessor = class()

function cGameProcessor:init(config,actionsProcessor)
    self.m_ap = actionsProcessor
    self.m_game = nil
    self.m_config = config
    self.m_config.adminPin = self.m_config.adminPin or 0
end

function cGameProcessor:startGameByConfig(gameConfig)
    if(gameConfig.module~=nil) then
        return self:startGame(gameConfig.caption,gameConfig.module,gameConfig.name.."_func",gameConfig.name)
    else
        return self:startGame(gameConfig.caption,"g"..gameConfig.name.."_mod",gameConfig.name.."_func",gameConfig.name)
    end
end

function cGameProcessor:startGameByName(gameBaseName)
    return self:startGame(gameBaseName,"g"..gameBaseName.."_mod",gameBaseName.."_func",gameBaseName)
end

function cGameProcessor:startGame(gameCaption, gameModuleName, gameFunctionName, gameConfigName)
    global:logln("Loading game "..gameCaption)
    screenItems:clear()
    screenItems:drawString("Loading Game",const.SCREEN_X/2,const.SCREEN_Y/2-FS_MEDIUM-2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    screenItems:drawString(gameCaption,const.SCREEN_X/2,const.SCREEN_Y/2,FS_MEDIUM,const.TEXT_ALIGN_CENTER)
    screen:displayScreenItems(1)

    local gm = require(gameModuleName)
    if(gm~=nil) then
        local gf = gm[gameFunctionName]
        if(gf~=nil) then
            local gameConfig = cConfig(gameConfigName)
            local config = gameConfig:load()
            global:logln("Game module loaded")
            self.m_game = gf(config)
            if(self.m_game~=nil) then
                self.m_game:startGame(self)
                global:logln("Game started")
                return true
            else
                global:logln("No game")
                return false
            end
        end
    end
    return false
end

function cGameProcessor:getParamsByConfig(gameConfig)
    if(gameConfig.module~=nil) then
        return self:getParams(gameConfig.module,gameConfig.name.."_setup_func",gameConfig.name)
    else
        return self:getParams("g"..gameConfig.name.."_mod",gameConfig.name.."_setup_func",gameConfig.name)        
    end
end

function cGameProcessor:getParamsByName(gameBaseName)
    return self:getParams("g"..gameBaseName.."_mod",gameBaseName.."_setup_func",gameBaseName)
end

function cGameProcessor:getParams(gameModuleName, gameFunctionName, gameConfigName)
    local gm = require(gameModuleName)
    if(gm~=nil) then
        local gf = gm[gameFunctionName]
        if(gf~=nil) then
            local result = {}
            result.configFile = cConfig(gameConfigName)
            result.config = result.configFile:load()
            result.params = gf()
            if(result.params~=nil and #result.params>0) then
                return result
            end
        end
    end
    return nil
end


function cGameProcessor:gameOver()
    if(self.m_game~=nil) then
        self.m_game:endGame()
    end
    screenItems:clear()
    screenItems:drawString("GAME OVER",const.SCREEN_X/2,const.SCREEN_Y/2,FS_LARGE,const.TEXT_ALIGN_CENTER_BOTH)
    screen:displayScreenItems()

    self.m_ap:setActions( { cActionBase({},nil,nil) } )
end