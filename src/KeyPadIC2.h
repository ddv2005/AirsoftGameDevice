#pragma once
#include <Keypad.h>
#include <Wire.h>
#include "PCF8574/PCF8574.h"

class cKeyPadI2C:public Keypad
{
protected:
    TwoWire *m_port;
    uint8_t m_address;
    PCF8574 m_pcf;
    bool m_force;
public:
    cKeyPadI2C(TwoWire *port, uint8_t address, char *userKeymap, byte *row, byte *col, byte numRows, byte numCols):Keypad(userKeymap,row,col,numRows,numCols),m_port(port),
        m_address(address),m_pcf(m_port,m_address)
    {
        for (byte r=0; r<numRows; r++) {
            m_pcf.pinMode(row[r],INPUT_PULLUP);
        }

        for (byte c=0; c<numCols; c++) {
            m_pcf.pinMode(col[c],OUTPUT);
        }
    }

    void begin()
    {
        m_force = true;
        m_pcf.begin();
    }

  	virtual void pin_mode(byte pinNum, byte mode)
    {
        //m_pcf.pinMode(pinNum,mode);
    }
	virtual void pin_write(byte pinNum, boolean level)
    {
        m_force = true;
        m_pcf.digitalWrite(pinNum,level);
    }

	virtual int  pin_read(byte pinNum)
    {
        return m_pcf.digitalRead(pinNum,m_force);
    }

};