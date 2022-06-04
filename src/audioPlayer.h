#pragma once
#include <Arduino.h>
#include "AudioFileSourceFS.h"
#include "AudioOutput.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "concurrency\Thread.h"
#include "esputils.h"
#include <list>

#define APE_PLAY_LIST   1
#define APE_STOP        2

struct sAudioPlayerEvent
{
    int8_t eid;
    int16_t param1;
    std::list<std::string> list;
};

typedef cProtectedQueue<sAudioPlayerEvent> cAudioPlayerEvents;

class cAudioPlayer
{
protected:
    typedef concurrency::ThreadFunctionTemplate<cAudioPlayer> cAudioPlayerThreadFunction;
    AudioOutput *m_out;
    cAudioPlayerEvents m_events;
    cAudioPlayerThreadFunction *m_thread;

    std::list<std::string> m_currentList;
    AudioFileSource *m_currentFile;
    AudioGenerator *m_mp3;

    void stopPlayer();
    void apThread(cAudioPlayerThreadFunction *thread);
public:    
    cAudioPlayer(AudioOutput *out);
    void begin();
    void sendEvent(const sAudioPlayerEvent &event)
    {
        m_events.push_item(event);
    }

    void sayNumber(uint16_t n, const char *file);
    void playFile(const char *file);
    void playFile2(const char *file1,const char *file2);
    void playStop();
};