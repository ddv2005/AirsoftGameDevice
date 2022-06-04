require "class"

cSoundProcessor = class()

function cSoundProcessor:init()
    self.m_folder = 10
end

function cSoundProcessor:play(f)
    global:playSound(self.m_folder,f)
end

function cSoundProcessor:playError()
    self:play(1)
end

function cSoundProcessor:playOK()
    self:play(2)
end

function cSoundProcessor:playClick()
    self:play(3)
end

function cSoundProcessor:playBomb()
    self:play(99)
end