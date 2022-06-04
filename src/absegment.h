#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <list>
#include "concurrency/Lock.h"
#include "concurrency/LockGuard.h"
#include "LED\Adafruit_LEDBackpack.h"

//Seglemt mode
#define ABSM_NONE    0
#define ABSM_TIMER   1
#define ABSM_HOLD    2
#define ABSM_STOPWATCH   3

class cBlinkingInfo
{
public:
    struct sBlinkStep
    {
        volatile int32_t position;
        volatile int16_t duration;
    };
    typedef std::list<sBlinkStep> cBlinkSteps;
    typedef std::list<sBlinkStep>::const_iterator cBlinkStepsIterator;

    int8_t m_blinkType; //-1 - On/Off, >=0 - Brightness
    cBlinkSteps m_steps; //sorted by position (1->0)
    cBlinkingInfo():m_blinkType(-1)
    {
    }

    void clear()
    {
        m_blinkType = -1;
        m_steps.clear();
    }

    void addStep(int32_t position, int16_t duration)
    {
        sBlinkStep step;
        step.duration = duration;
        step.position = position;
        m_steps.push_back(step);
    }

};

class abSegment
{
protected:
    union abState
    {
        struct 
        {
            int64_t m_start;
            int64_t m_stop;
            int32_t m_current;
            bool m_colon;
            unsigned long m_startTime;
        }absTimer;
        struct 
        {
            int32_t m_time;
            bool m_colon;
        }absHold;
    };

    Adafruit_7segment m_display;
    concurrency::Lock m_lock;
    volatile uint16_t m_mode;
    volatile abState m_state;
    cBlinkingInfo m_blinkInfo;
    volatile unsigned long m_blinkTime;
    volatile bool m_blinkValue;
    volatile int32_t m_lastBlinkDelay;

    volatile unsigned long m_lastUpdate;
    
public:
    abSegment():m_mode(ABSM_NONE)
    {
        memset((void*)&m_state,0,sizeof(m_state));
        m_lastUpdate = 0;
        m_blinkValue = false;
        m_blinkTime = 0;
    };

    void begin(TwoWire *wire, uint8_t address);

    void setTimer(int32_t start, int32_t stop, const cBlinkingInfo &blinkInfo);
    void setStopWatch(int32_t start, int32_t stop, const cBlinkingInfo &blinkInfo);
    void setHold(int32_t time, bool col, const cBlinkingInfo &blinkInfo);
    void setNone();
    int32_t getTimer();
    uint16_t getMode()
    {
        return m_mode;
    }

    void loop(unsigned long now);

    void resetBlinkInfo()
    {
        m_blinkTime = 0;
        m_blinkValue = false;
        m_lastBlinkDelay = 0;
    }
};