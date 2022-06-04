require "class"

RT_ALARM = 0

cRelay = class()

function cRelay:init()
    self.m_type = RT_ALARM
    self.m_alarm_count = 1
    self.m_alarm_on = 2000
    self.m_alarm_off = 2000
    self.m_alarm_state = 0
    self.m_alarm_alarm_restart = 0
end

function cRelay:setAlarm(_cnt, _on, _off)
    self.m_type = RT_ALARM
    self.m_alarm_count = _cnt
    self.m_alarm_on = _on
    self.m_alarm_off = _off
end

function cRelay:pulse()
    if(self.m_type==RT_ALARM) then
        global:relayPulses(self.m_alarm_count,self.m_alarm_on,self.m_alarm_off)
    end
end

function cRelay:pulseCustom(count, alarm_on, alarm_off)
    if(self.m_type==RT_ALARM) then
        global:relayPulses(count,alarm_on,alarm_off)
    end
end
