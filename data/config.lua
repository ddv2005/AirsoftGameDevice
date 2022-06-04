require "class"

cConfig = class()

function cConfig:init(name)
	self.m_name = name
end

function cConfig:load()
    local f = io.open("/config/"..self.m_name, "r")
    if(f ~= nil) then
      local content = f:read("*all")
      io.close(f)
      if(content ~= nil) then
        local o = global:strToLua(content)
        if(o ~= nil) then
            return o
        else
            return {}
        end
      else
        return {}
      end
    else
      return {}
    end
end

function cConfig:save(o)
    local content = global:luaToStr(o)
    local f = io.open("/config/"..self.m_name, "w+")
    f:write(content)
    io.close(f)
end