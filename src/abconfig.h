#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class cConfig
{
protected:
    virtual void initDefaults()=0;
    virtual bool checkConfig()=0;
    virtual void saveConfigValues(JsonDocument &json)=0;
    virtual void loadConfigValues(JsonDocument &json)=0;
public:
    cConfig();
    virtual bool loadConfig(std::istream &stream,bool binary=false);
    virtual void saveConfig(std::ostream &stream,bool binary=false);

    virtual void loadConfig(const char * filename);
    virtual void saveConfig(const char * filename);
};