#include "absegment.h"
#include "config.h"

void abSegment::begin(TwoWire *wire, uint8_t address)
{
    m_display.begin(wire,address);
    m_display.clear();
    m_display.writeDisplay();
    setNone();
}

void abSegment::setStopWatch(int32_t start, int32_t stop, const cBlinkingInfo &blinkInfo)
{
    concurrency::LockGuard lock(&m_lock);
    m_mode = ABSM_STOPWATCH;
    m_state.absTimer.m_start = start;
    m_state.absTimer.m_stop = stop;
    m_state.absTimer.m_current = -1;
    m_state.absTimer.m_startTime = millis();
    m_state.absTimer.m_colon = false;
    m_lastUpdate = 0;
    m_blinkInfo = blinkInfo;
    resetBlinkInfo();
}

void abSegment::setTimer(int32_t start, int32_t stop, const cBlinkingInfo &blinkInfo)
{
    concurrency::LockGuard lock(&m_lock);
    m_mode = ABSM_TIMER;
    m_state.absTimer.m_start = start;
    m_state.absTimer.m_stop = stop;
    m_state.absTimer.m_current = -1;
    m_state.absTimer.m_startTime = millis();
    m_state.absTimer.m_colon = false;
    m_lastUpdate = 0;
    m_blinkInfo = blinkInfo;
    resetBlinkInfo();
}

void abSegment::setHold(int32_t time, bool col, const cBlinkingInfo &blinkInfo)
{
    concurrency::LockGuard lock(&m_lock);
    m_mode = ABSM_HOLD;
    m_state.absHold.m_colon = col;
    m_state.absHold.m_time = time;
    m_lastUpdate = 0;
    m_blinkInfo = blinkInfo;
    resetBlinkInfo();
}

int32_t abSegment::getTimer()
{
    concurrency::LockGuard lock(&m_lock);
    switch (m_mode)
    {
        case ABSM_STOPWATCH:
        {
            int32_t result = m_state.absTimer.m_start+(millis()-m_state.absTimer.m_startTime);
            if(result>m_state.absTimer.m_stop)
                result = m_state.absTimer.m_stop;
            return result;
        }

        case ABSM_TIMER:
        {
            int32_t result = m_state.absTimer.m_start-(millis()-m_state.absTimer.m_startTime);
            if(result<m_state.absTimer.m_stop)
                result = m_state.absTimer.m_stop;
            return result;
        }

        case ABSM_HOLD:
            return m_state.absHold.m_time;

        default:
            return 0;
    }
}

void abSegment::setNone()
{
    concurrency::LockGuard lock(&m_lock);
    m_mode = ABSM_NONE;
    m_lastUpdate = 0;
    m_blinkInfo.clear();
    resetBlinkInfo();
}

void abSegment::loop(unsigned long now)
{
    int32_t blinkPosition = 0;
    bool updated = false;
    bool canprocess = true;
    if(m_lastUpdate==0)
    {
        m_display.setLCDOn(true);
        m_display.setBrightness(15);
    }
    m_lock.lock();
    switch (m_mode)
    {
        case ABSM_NONE:
        {
            if(m_lastUpdate==0)
            {
                m_display.clear();
                updated = true;
            }
            break;
        }

        case ABSM_HOLD:
        {
            if(m_lastUpdate==0)
            {
                int32_t value = m_state.absHold.m_time/1000;
                blinkPosition = value;
                uint8_t h = value / 3600;
                uint8_t m = (value % 3600) / 60;
                uint8_t s = value % 60;
                if(h>0)
                {
                    m_display.writeDigitNum(0,100);
                    m_display.writeDigitNum(1,h%10,true);
                    m_display.drawColon(m_state.absHold.m_colon);
                    m_display.writeDigitNum(3,m/10);
                    m_display.writeDigitNum(4,m%10);
                }
                else
                {
                    m_display.writeDigitNum(0,m/10);
                    m_display.writeDigitNum(1,m%10);
                    m_display.drawColon(m_state.absHold.m_colon);
                    m_display.writeDigitNum(3,s/10);
                    m_display.writeDigitNum(4,s%10);
                }
                updated = true;
            }
            break;
        }

        case ABSM_STOPWATCH:
        case ABSM_TIMER:
        {
            if((now-m_lastUpdate)>=50)
            {
                bool colon = ((now-m_state.absTimer.m_startTime)%1000)<=450;
                int64_t value = 0;
                if(m_mode==ABSM_STOPWATCH)
                {
                    value = m_state.absTimer.m_start+(now-m_state.absTimer.m_startTime);
                    if(value>m_state.absTimer.m_stop)
                    {
                        value = m_state.absTimer.m_stop;
                        colon = true;
                    }
                }
                else
                {
                    value = m_state.absTimer.m_start-(now-m_state.absTimer.m_startTime);
                    if(value<m_state.absTimer.m_stop)
                    {
                        value = m_state.absTimer.m_stop;
                        colon = true;
                    }
                }
                blinkPosition = value;
                value /= 1000;
                updated = (value != m_state.absTimer.m_current) || (colon!=m_state.absTimer.m_colon);
                if(updated)
                {
                    m_state.absTimer.m_current = value;
                    m_state.absTimer.m_colon = colon;
                    uint8_t h = value / 3600;
                    uint8_t m = (value % 3600) / 60;
                    uint8_t s = value % 60;
                    if(h>0)
                    {
                        m_display.writeDigitNum(0,100);
                        m_display.writeDigitNum(1,h%10,true);
                        m_display.drawColon(colon);
                        m_display.writeDigitNum(3,m/10);
                        m_display.writeDigitNum(4,m%10);
                    }
                    else
                    {
                        m_display.writeDigitNum(0,m/10);
                        m_display.writeDigitNum(1,m%10);
                        m_display.drawColon(colon);
                        m_display.writeDigitNum(3,s/10);
                        m_display.writeDigitNum(4,s%10);
                    }
                }
            }
            else
            {
                canprocess = false;
            }
            
            break;
        }        
        
        default:
            break;
    }
    int32_t blinkDelay = 0;
    if(canprocess)
    {
        if(m_blinkInfo.m_steps.size())
        {
            for(cBlinkingInfo::cBlinkStepsIterator itr = m_blinkInfo.m_steps.begin(); itr!=m_blinkInfo.m_steps.end(); itr++)
            {
                if(itr->position>=blinkPosition)
                {
                    blinkDelay = itr->duration;
                }
                else
                    break;
            }
        }
    }
    m_lock.unlock();
    if(canprocess)
    {
        if(updated || (m_lastUpdate==0))
        {
            m_lastUpdate = millis();
            m_display.writeDisplay();
        }
        if(blinkDelay>0)
        {
            m_lock.lock();
            if((m_blinkTime<now) && (now-m_blinkTime)>=blinkDelay)
            {
                if(m_lastBlinkDelay!=blinkDelay)
                {
                    m_blinkTime = (now/(blinkDelay*2)+1)*blinkDelay*2;
                    m_blinkValue = true;
                }
                else
                    m_blinkTime = m_blinkTime+((now-m_blinkTime)/blinkDelay)*blinkDelay;
                m_blinkValue = !m_blinkValue;
                if(m_blinkInfo.m_blinkType<0)
                    m_display.setLCDOn(m_blinkValue);
                else
                    m_display.setBrightness(m_blinkValue?15:m_blinkInfo.m_blinkType);
                m_lastBlinkDelay = blinkDelay;
            }
            m_lock.unlock();
        }
        else
        {
            m_lastBlinkDelay = blinkDelay;
            if(!m_blinkValue)
            {
                m_lock.lock();
                m_blinkValue = true;
                m_lock.unlock();
                m_display.setLCDOn(true);
                m_display.setBrightness(15);
            }
        }
    }
}