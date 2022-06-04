#pragma once

#include "config.h"
#include <Arduino.h>
#ifdef USE_I2S
#include <FS.h>
#include "AudioOutputI2S.h"
#include "audioPlayer.h"
class cPlayer
{
protected:
    AudioOutputI2S *m_audioOut;
    cAudioPlayer *m_player;
public:
    cPlayer():m_audioOut(NULL),m_player(NULL)
    {

    }

    void begin(uint8_t pin1,uint8_t pin2, uint8_t pin3)
    {
        m_audioOut = new AudioOutputI2S();
        m_audioOut->SetOutputModeMono(true);
        m_audioOut->SetPinout(pin1,pin2,pin3);
        m_audioOut->SetGain(1);
        m_audioOut->begin();

        m_player = new cAudioPlayer(m_audioOut);
        m_player->begin();
    }

    void play(uint8_t folder, uint8_t file)
    {
        m_player->playFile(String("/sfx/"+String(folder)+"/"+String(file)+".wav").c_str());
    }

    void stop()
    {
        m_player->playStop();
    }    
};

#else
#ifdef DFPLAYER_MINI
#include <DFRobotDFPlayerMini.h>
#else
#include <JQ8400_Serial.h>
#endif

class cPlayer
{
protected:
    HardwareSerial *m_serial;
#ifdef DFPLAYER_MINI
    DFRobotDFPlayerMini *m_player;
#else
    JQ8400_Serial *m_player;
#endif
public:
    cPlayer():m_serial(NULL),m_player(NULL)
    {

    }

    void begin(HardwareSerial *serial)
    {
        m_serial = serial;
#ifdef DFPLAYER_MINI
        m_player = new DFRobotDFPlayerMini();
        m_player->begin(*m_serial,true, false);
        m_player->stop();
        m_player->setTimeOut(200);
        m_player->volume(30);
#else
        m_player = new JQ8400_Serial(*m_serial);
        delay(1000);
        m_player->setVolume(30);
#endif        
    }

    void play(uint8_t folder, uint8_t file)
    {
#ifdef DFPLAYER_MINI
        m_player->playFolder(folder, file);
#else
        m_player->playFileNumberInFolderNumber(folder,file);
#endif        
    }

    void stop()
    {
        m_player->stop();
    }

};
#endif