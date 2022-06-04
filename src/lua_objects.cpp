#include "lua_objects.h"
#include <lua_utils.h>
#include <esp_timer.h>

cLuaScreen::cLuaScreen(Screen *screen):m_screen(screen)
{

}

void cLuaScreen::setup(sol::state &lua)
{
    lua.new_simple_usertype<cScreenItems>("cScreenItems",
        "new",sol::no_constructor,
        LUA_FUNC(cScreenItems,clear),
        LUA_FUNC(cScreenItems,drawString),
		LUA_FUNC(cScreenItems,drawProgress)
        );

    lua.new_simple_usertype<cLuaScreen>("cLuaScreen",
        "new",sol::no_constructor,
        LUA_FUNC(cLuaScreen,displayScreenItems),
		LUA_FUNC(cLuaScreen,displayScreenOverlayItems),
        LUA_FUNC(cLuaScreen,getScreenItems)
        );
}

cScreenItems &cLuaScreen::getScreenItems()
{
    return m_items;
}

void cLuaScreen::displayScreenOverlayItems(int8_t forceUpdate)
{
    m_screen->m_overlayItems = m_items;
	if(forceUpdate)
		m_screen->forceUpdate();
}

void cLuaScreen::displayScreenItems(int8_t forceUpdate)
{
    m_screen->m_items = m_items;
	if(forceUpdate)
		m_screen->forceUpdate();
}


//==============================================
cLuaTimers::cLuaTimers()
{
    m_lastTimerID = 0;
}

int64_t cLuaTimers::now()
{
    return esp_timer_get_time()/1000LL;
}

int64_t	cLuaTimers::getTimersWait()
{
	int64_t nowc = now();
	int64_t result = 0xFFFFFFFFFFFF;
	auto itr = m_timers.begin();
	for(;itr!=m_timers.end();itr++)
	{
		int64_t time = itr->second.interval-itr->second.getTimerWait(nowc);
		if(time<0)
			time = 0;
		if(result>time)
			result = time;
	}
	return result;
}

void cLuaTimers::processTimers()
{
	int64_t nowc = now();
	auto itr = m_timers.begin();
	while(itr!=m_timers.end())
	{
		if(itr->second.interval<=itr->second.getTimerWait(nowc))
		{
			itr->second.last_fire_time = nowc;
			sol::function callback =itr->second.callback;
			try{
				if(callback.valid())
				{
					callback();
				}
			}catch(std::exception &e)
			{
				DEBUG_MSG("Timer exception %s\n",e.what());
			}
			nowc = now();
			itr = m_timers.begin();
		}
		else
			itr++;
	}
}

uint32_t cLuaTimers::addTimer(int64_t interval, sol::function callback)
{
	if(callback.valid()&&(interval>0))
	{
		lua_timer_t timer;
		timer.id = ++m_lastTimerID;
		timer.interval = interval;
		timer.first_fire_time = timer.last_fire_time = now();
		timer.callback = callback;
		m_timers[timer.id] = timer;
		return timer.id;
	}
	else
		return 0;
}

void cLuaTimers::deleteTimer(uint32_t tid)
{
    m_timers.erase(tid);
}

void cLuaTimers::setup(sol::state &lua)
{
    lua.new_simple_usertype<cLuaTimers>("cLuaTimers",
        "new",sol::no_constructor,
        LUA_FUNC(cLuaTimers,addTimer),
        LUA_FUNC(cLuaTimers,deleteTimer)
        );
}
