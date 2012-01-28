#pragma once
#ifndef LUA_FUNCS_H
#define LUA_FUNCS_H

#include <string>
#include <microhttpd.h>
#include <boost/thread.hpp>

#define LUAJIT

#ifdef LUAJIT

//#include "luajit.h"
#include "lua.hpp"

#define LUAJITSETUP(L) luaJIT_setmode(L, 0, LUAJIT_MODE_ON)

#else
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#define LUAJITSETUP(L)
#endif

#include "Luacreator.h"

struct connection_t
{
	std::string url;
	std::string method;
	std::string version;
	std::string response;
	std::string ip;
	int errcode;
};


extern int l_GetCurrentTime(lua_State* L);
extern double GetCurrentTime();

extern int l_ResetMicroTime(lua_State* L);
extern int l_MicroTime(lua_State* L);
extern void MicroTime_Free(lua_State* L);

extern int l_Print(lua_State*);
extern int l_EscapeHTML(lua_State* L);
extern int l_DirExists(lua_State *L);
extern int l_FileExists(lua_State *L);
extern int l_ParseLuaString(lua_State* L);
extern int l_GenerateSessionID(lua_State* L);


extern void LoadMods(lua_State* L);

#define LOCK boost::mutex::scoped_lock l(*GetLock())
extern boost::mutex* GetLock();

extern int l_Lock(lua_State* L);

#endif // LUA_FUNCS_H
