require "sounds"
require "relay"

screen = global:getScreen()
screenItems = screen:getScreenItems()

FS_SMALL = 10
FS_MEDIUM = 16
FS_LARGE = 24

FS_MONO = 100

PIN_SIZE = 4

ST_INT  = 1
ST_BOOL = 2
ST_CODE = 3

sp = cSoundProcessor()
relay = cRelay()

function unrequire(m)
	package.loaded[m] = nil
    _G[m] = nil
end
