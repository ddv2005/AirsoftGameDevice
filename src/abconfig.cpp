#include "config.h"
#include "abconfig.h"
#include <fstream>

cConfig::cConfig()
{
}

bool cConfig::loadConfig(std::istream &stream,bool binary)
{
    try{
        StaticJsonDocument<1024> json;
        DeserializationError error;
        if(binary)
            error = deserializeMsgPack(json,stream);
        else
            error = deserializeJson(json,stream);
        if(error)
        {
            DEBUG_MSG("cConfig loading error %s\n",error.c_str());
            return false;
        }
        else
        {
            initDefaults();
            loadConfigValues(json);
        }
    }catch(...)
    {
        return false;
    }
    return true;
}

void cConfig::saveConfig(std::ostream &stream,bool binary)
{
    StaticJsonDocument<1024> json;
    saveConfigValues(json);

    try{
        if(binary)
            serializeMsgPack(json,stream);
        else
            serializeJson(json,stream);
    }catch(...)
    {
    }
}


void cConfig::loadConfig(const char * filename)
{
    try{
        std::ifstream file(filename,std::ifstream::binary);
        if(!loadConfig(file))
            saveConfig(filename);
    }catch(...)
    {
        saveConfig(filename);
    }
    if(checkConfig())
        saveConfig(filename);
}

void cConfig::saveConfig(const char * filename)
{
    try{
        std::ofstream file(filename,std::ofstream::binary);
        saveConfig(file);
    }catch(...)
    {
    }
}
