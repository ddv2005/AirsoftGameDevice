#pragma once
#include <sol/sol.hpp>
#include <map>

class cLuaObjectBase
{
public:
    cLuaObjectBase(){};
    virtual ~cLuaObjectBase(){};
    virtual void setup(sol::state &lua)=0;
};

#include "abscreen.h"

class cLuaScreen: public cLuaObjectBase
{
protected:
    Screen *m_screen;
    cScreenItems m_items;
public:
    cLuaScreen(Screen *screen);
    virtual void setup(sol::state &lua);

    cScreenItems &getScreenItems();
    void displayScreenItems(int8_t forceUpdate);
	void displayScreenOverlayItems(int8_t forceUpdate);
};

struct lua_timer_t
{
	uint32_t id;
	int64_t	interval;
	int64_t first_fire_time;
	int64_t last_fire_time;
	sol::function callback;

	int64_t getTimerWait(int64_t now)
	{

		return now - first_fire_time - ((last_fire_time-first_fire_time)/interval)*interval;
	}
};

class cLuaTimers: public cLuaObjectBase
{
protected:
	std::map<uint32_t,lua_timer_t> m_timers;
	uint32_t m_lastTimerID;
    int64_t now();
public:
    cLuaTimers();
    virtual void setup(sol::state &lua);

	int64_t	getTimersWait();
	void processTimers();

	uint32_t addTimer(int64_t interval, sol::function callback);
	void deleteTimer(uint32_t tid);    
};

