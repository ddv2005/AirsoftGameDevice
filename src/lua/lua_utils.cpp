#include "lua_utils.h"
#include "json.hpp"


static sol::object jsonToLuaObject(const nlohmann::json & j, sol::state & lua)
{
        if (j.is_null())
        {
                return sol::nil;
        }
        else if (j.is_boolean())
        {
                return sol::make_object(lua, j.get<bool>());
        }
        else if (j.is_number_integer())
        {
                return sol::make_object(lua, j.get<int>());
        }
        else if (j.is_number())
        {
                return sol::make_object(lua, j.get<double>());
        }
        else if (j.is_string())
        {
                return sol::make_object(lua, j.get<std::string>().c_str());
        }
        else if (j.is_object())
        {
                sol::table obj = lua.create_table();
                for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it)
                {
                        obj[it.key().c_str()] = jsonToLuaObject(*it, lua);
                }
                return obj.as<sol::object>();
        }
        else if (j.is_array())
        {
                sol::table obj = lua.create_table();
                unsigned long i = 0;
                for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it)
                {
                        obj[i++] = jsonToLuaObject(*it, lua);
                }
                return obj;
        }
        return sol::nil;
}

sol::object strJsonToLuaObject(const std::string& str, sol::state & lua)
{
        auto j = nlohmann::json::parse(str.c_str());
        return jsonToLuaObject(j,lua);
}

nlohmann::json luaObjectToJson(const sol::object &obj)
{
        if(obj.is<int>())
        {
                return nlohmann::json(obj.as<int>());
        }
        else
                if(obj.is<std::string>())
                {
                        return nlohmann::json(obj.as<std::string>());
                }
                else
                        if(obj.is<double>())
                        {
                                return nlohmann::json(obj.as<double>());
                        }
                        else
                                if(obj.is<bool>())
                                {
                                        return nlohmann::json(obj.as<bool>());
                                }
                                else
                                        if(obj.is<sol::table>())
                                        {
                                                auto j = nlohmann::json();
                                                sol::table t = obj;
                                                t.for_each([&](sol::object const& key, sol::object const& value) {
                                                                j[key.as<std::string>()] =  luaObjectToJson(value);
                                                    });

                                                return j;
                                        }
                                        else
                                                return nlohmann::json();
}

std::string luaObjectToJsonStr(const sol::object &obj)
{
        auto j = luaObjectToJson(obj);
        return j.dump();
}

