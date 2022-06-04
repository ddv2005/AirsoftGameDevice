#pragma once
#include <Arduino.h>
#include <NeoPixelBus.h>
#include "config.h"

#ifdef NEOPIXELS_WRGB
typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt1Ws2812xMethod> esp32NeoPixelBus;
#else
//typedef NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1Ws2812xMethod> esp32NeoPixelBus;
typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod> esp32NeoPixelBus;
#endif
//typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod> esp32NeoPixelBus;
class cNeopixels
{
protected:
    uint8_t m_count;
    uint8_t m_p;
    uint8_t *m_map;
    esp32NeoPixelBus *m_neo;    
public:
    cNeopixels()
    {
        m_map = NULL;
        m_neo = NULL;
        m_count = 0;
        m_p = 0;
    }

    void begin(esp32NeoPixelBus *neo, uint8_t count, uint8_t p, uint8_t* map)
    {
        m_count = count;
        m_p = p;
        m_map = map;
        m_neo = neo;
    }

    void setPixelColor(uint8_t index, RgbColor color)
    {
        if(m_neo&&(index<m_count))
        {
            for(int i=0; i<m_p; i++)
            {
                m_neo->SetPixelColor(m_map[index*m_p+i],color);
            }
        }
    }

    void show()
    {
        if(m_neo)
            m_neo->Show();
    }

    void clear()
    {
        m_neo->ClearTo(RgbColor(0,0,0));
    }

    void drawGuage(int level, RgbColor color)
    {
        drawGuageEx(level,color,RgbColor(0));
    }

    void drawGuageEx(int level, RgbColor color1, RgbColor color2)
    {
        int li = (m_count-1)*level/100;
        for(int i=0; i<m_count; i++)
        {
            if(i<=li)
                setPixelColor(i,color1);
            else
                setPixelColor(i, color2);
        }
    }
};