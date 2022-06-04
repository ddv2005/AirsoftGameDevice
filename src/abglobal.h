#pragma once
#include <Arduino.h>
#include <power.h>
#include <Wire.h>
#include "power.h"
#include "GPSStatus.h"

class abGlobal
{
protected:
    Power *m_power;

    virtual void initPower()
    {
        if (m_has_axp192)
        {
            m_power = new Power();
            m_power->setup();
            m_power->setStatusHandler(m_powerStatus);
            m_powerStatus->observe(&m_power->newStatus);
        }
    }
public:
    bool m_has_axp192;
    bool m_has_oled;
    PowerStatus *m_powerStatus;
    GPSStatus *m_gpsStatus;

    abGlobal()
    {
        m_has_axp192 = m_has_oled = false;
        m_powerStatus = new PowerStatus();
        m_power = NULL;
        m_gpsStatus = new GPSStatus();

    }
    virtual ~abGlobal()
    {
        delete m_gpsStatus;
        delete m_powerStatus;
        if(m_power)
            delete m_power;
    }

    virtual void init()
    {
        initPower();
    }

    virtual void loop()
    {
        concurrency::periodicScheduler.loop();
        if(m_power)
            m_power->loop();
    }

    Power *getPower(){
        return m_power;
    }
};
