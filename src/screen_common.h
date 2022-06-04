#pragma once
#include <stdint.h>
#include <list>
#include <vector>
#include <string>
#include <string.h>

//Screen item types
#define SIT_STRING      1
#define SIT_PROGRESS    2

#define SCREEN_Y_OFFSET 10
#define SCREEN_X    128
#define SCREEN_Y    (64-SCREEN_Y_OFFSET)

struct sScreenItem
{
    uint8_t itemType;
    std::string strData;
    uint8_t x;
    uint8_t y;
    union {
        struct {
            uint8_t fontSize;
            uint8_t aligment;
        } stringData;
        struct {
            uint16_t width;
            uint16_t height;
            uint8_t progress;
        } progressData;
    } itemData;

    sScreenItem(uint8_t itype):itemType(itype)
    {
        x = y = 0;
        memset(&itemData,0,sizeof(itemData));
    }

    int16_t getSize()
    {
        return strData.length()+sizeof(uint8_t)*4+sizeof(itemData);
    }

    void serialize(uint8_t *data)
    {
        *data = itemType; data++;
        *data = x; data++;
        *data = y; data++;
        memcpy(data,&itemData,sizeof(itemData)); data+=sizeof(itemData);
        *data = strData.length(); data++;
        memcpy(data,strData.c_str(),strData.length());
    }

    void deserialize(const uint8_t *data)
    {
        itemType = *data; data++;
        x = *data; data++;
        y = *data; data++;
        memcpy(&itemData,data, sizeof(itemData)); data+=sizeof(itemData);
        uint8_t size = *data; data++;
        strData.assign((const char*)data,size);
    }    
};

typedef std::list<sScreenItem> cScreenItemsBase;

class cScreenItems
{
protected:
    cScreenItemsBase m_list;
public:
    cScreenItems()
    {}

    void drawString(std::string str, uint8_t x, uint8_t y, uint8_t fontSize, uint8_t aligment)
    {
        sScreenItem item(SIT_STRING);
        item.strData = str;
        item.x = x;
        item.y = y;
        item.itemData.stringData.fontSize = fontSize;
        item.itemData.stringData.aligment = aligment;
        m_list.push_back(item);
    }

    void drawProgress(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress)
    {
        sScreenItem item(SIT_PROGRESS);
        item.x = x;
        item.y = y;
        item.itemData.progressData.width = width;
        item.itemData.progressData.height = height;
        item.itemData.progressData.progress = progress;                
        m_list.push_back(item);
    }

    void clear()
    {
        m_list.clear();
    }

    cScreenItemsBase &getList()
    {
        return m_list;
    }

    int16_t getSize()
    {
        int16_t result = sizeof(uint8_t)*(m_list.size()+1);
        for(auto itr = m_list.begin(); itr != m_list.end(); itr++)
            result += itr->getSize();
        return result;
    }

    void serialize(uint8_t *data)
    {
        *data = m_list.size(); data++;
        for(auto itr = m_list.begin(); itr != m_list.end(); itr++)
        {
            uint8_t size = itr->getSize();
            *data = size; data++;
            itr->serialize(data);
            data+=size;
        }
    }

    void deserialize(const uint8_t *data)
    {
        clear();
        uint8_t count = *data; data++;
        for(int i=0; i<count; i++)
        {
            sScreenItem item(0);
            uint8_t size = *data; data++;
            item.deserialize(data);
            data+=size;
            m_list.push_back(item);
        }
    }
};

