#pragma once
#include <sol/sol.hpp>

#define LUA_FUNC(class_name,name) #name,&class_name::name
#define LUA_GFUNC(name) #name,&::name

sol::object strJsonToLuaObject(const std::string& str, sol::state & lua);
std::string luaObjectToJsonStr(const sol::object &obj);
