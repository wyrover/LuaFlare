#include "LuaFuncs.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <iostream>

#include <dirent.h>

using namespace std;

void LoadMods(lua_State* L, string sdir)
{
	DIR *dir;
	struct dirent *ent;
	dir = opendir (sdir.c_str());
	if (dir != NULL)
	{
		while ((ent = readdir (dir)) != NULL)
		{
			if(ent->d_type == DT_DIR)
			{
				if(string(ent->d_name) == "..") continue;
				if(string(ent->d_name) == ".") continue;
				string newdir = sdir + ent->d_name + "/";
				
				LoadMods(L, newdir);
			}
			else
			{
				string lfile = sdir + ent->d_name;
				printf ("Loading %s\n", lfile.c_str());
				
			}
		}
		closedir (dir);
	}
}

void LoadMods(lua_State* L)
{
	LoadMods(L, "mods/");
}

int l_Print(lua_State* L)
{
	const char* str = luaL_checkstring(L, 1);
	cout << str;
	
	return 0;
}

int l_EscapeHTML(lua_State* L)
{
	string in = luaL_checkstring(L, 1);
	
	string buf;
	buf.reserve(in.size());
	for(size_t pos = 0; pos != in.size(); ++pos)
	{
		switch(in[pos])
		{
			case '&':	buf.append("&amp;");		break;
			case '\"':	buf.append("&quot;");	break;
			case '\'':	buf.append("&apos;");	break;
			case '<':	buf.append("&lt;");		break;
			case '>':	buf.append("&gt;");		break;

			default:	buf.append(1, in[pos]);	break;
		}
	}
	in.swap(buf);
	
	lua_pushstring(L, in.c_str());
	return 1;
}

void PrintTable(lua_State *L, int Depth)
{
	char* tabs = new char[Depth + 1];
	
	for(int i = 0; i < Depth; i++)
		tabs[i] = '\t';
	tabs[Depth] = '\0';
	
	lua_pushnil(L);
	
	while(lua_next(L, -2) != 0)
	{
		if(lua_isstring(L, -1))
			printf("%s%s = %s\n", tabs, lua_tostring(L, -2), lua_tostring(L, -1));
		else if(lua_isnumber(L, -1))
			printf("%s%s = %d\n", tabs, lua_tostring(L, -2), lua_tonumber(L, -1));
		else if(lua_istable(L, -1))
		{
			printf("%s%s:\n", tabs, lua_tostring(L, -2));
			PrintTable(L, Depth + 1); // TODO: Refrence to self
		}

		lua_pop(L, 1);
	}
	
	delete [] tabs;
}

int l_PrintTable(lua_State *L)
{
	return 0;
}

int l_DirExists(lua_State *L)
{
	const char* str = luaL_checkstring(L, 1);
	
	struct stat st;
	bool exists = stat(str, &st) == 0;
	
	lua_pushboolean(L, exists);
	return 1;
}

int l_FileExists(lua_State *L)
{
	return 0;
}

#define PARSEMODE_OUTOFLUA 0
#define PARSEMODE_INLUA 1
#define PARSEMODE_INCOMMENT 2

// <?lua ?> support
/*
This function basically turns:

Hello, <b><?lua write(GET.NAME or Unknown)?></b>.

Into:

write(false, [[Hello, <b>]])write(GET.NAME or Unknown)write(false,[[</b>.]])

It must not break under comments, strings or any other conditions like:

	-- Hello, ?>/<?lua world.
	func("Hello, ?>/<?lua world")
	func('Hello, ?>/<?lua world')
	func([[Hello, ?>/<?lua world]])
	
Escaping must work too:
	
	func("Just testing \") something")
	And this must work too:
	
	lua = [[ This is a test \]]
*/
int l_ParseLuaString(lua_State* L)
{
	string inlua = luaL_checkstring(L, 1);
	string outlua;
	
	outlua.reserve(inlua.size());

	int parsemode = PARSEMODE_OUTOFLUA;
	unsigned int inluastart = 0;
	outlua += "write(false,[[";
	
	unsigned int i = 0;
	
	while(i < inlua.length())
	{
		if(parsemode == PARSEMODE_OUTOFLUA)
		{
			while(i < inlua.length())
			{
				char x = inlua[i];
				if((i < inlua.length() - 4) && x == '<' && inlua[i+1] == '?' && inlua[i+2] == 'l' && inlua[i+3] == 'u' && inlua[i+4] == 'a')
				{
					inluastart = i;
					i += 5;
					outlua += "]])";
					parsemode = PARSEMODE_INLUA;
					break; // New parse mode!
				}
				else if(x == ']') 	// Major bloody hack D:
				{					// No other way, you can't put a ] in a literal string
					
					outlua += "]] .. \"]\" .. [[";
					i += 1;
				}
				else if(x == '[')
				{
					outlua += "]] .. \"[\" .. [[";
					i += 1;
				}
				else
				{
					outlua += x;
					i++;
				}
			}
		}
		else if(parsemode == PARSEMODE_INLUA)
		{
			while(i < inlua.length())
			{
				char x = inlua[i];
				
				if(x == '?' && inlua[i+1] == '>')
				{
					parsemode = PARSEMODE_OUTOFLUA;
					outlua += "write(false, [[";
					i+= 2;
					break;
				}
				else if(x == '"' || x == '\'' || (x == '[' && inlua[i+1] == '[') )
				{
					char exitnode = x;
					if(x == '[')
					{
						exitnode = ']';
						i++;
						outlua+= "[[";
					}
					else outlua += x;
					
					i++;
					if(x == ']') i++;
					
					while(i < inlua.length())
					{
						char y = inlua[i];
						
						if(y == '\\' && exitnode != ']') // Escape the next char, but only if we're not in a literal string
						{
							outlua += y;
							outlua += inlua[i+1];
							i += 2;
							continue;
						}
						
						outlua += y;
						i++;
						
						if(y == exitnode)
						{
							if(y == ']')
							{
								if(inlua[i] != ']')
									continue;
								outlua += "]";
								i++;
							}
							break;
						}
					}
				}
				else if(x == '-' && inlua[i+1] == '-' && i < inlua.length() - 1) // -1 on len to prevent crash if file ends with "-"
				{
					// comments
					outlua += "--";
					i+= 2;
					
					bool multiline = false;
					
					if(inlua[i] == '[' && inlua[i+1] == '[' )
					{
						outlua += "[[";
						i+= 2;
						multiline = true;
					}
					
					while(i < inlua.length())
					{
						char y = inlua[i];
						outlua += y;
						
						if(!multiline && (y == '\n' || y == '\r'))
							break;
							
						if(multiline && y == ']' && inlua[i + 1] == ']')
						{
							outlua += "]";
							i+=2;
							break;
						}
						
						i++;
					}
				}
				else
				{
					outlua += x;
					i++;
				}
			}
		}
		
	}
	
	outlua += "]])";
	
	if(parsemode == PARSEMODE_INLUA)
	{
		string offendingline = "";
		int linecount = 1;
		
		for(i = 0; i < inluastart; i++)
		{
			char x = inlua[i];
			if(x == '\r' && inlua[i+1] == '\n' && i < inluastart - 1)
			{
				linecount++;
				i++;
			}
			else if(x == '\r')
				linecount++;
			else if(x == '\n')
				linecount++;
		}
		
		while(i < inlua.length())
		{
			char x = inlua[i];
			if(x == '\n' || x == '\r')
				break;
			i++;
			offendingline += x;
		}
		// now we have the line number and line
		
		stringstream ss;
		ss << linecount;
		
		string error = "'?>' expected near '" + offendingline + "' (line " + ss.str() + ")";
		
		lua_pushnil(L);
		lua_pushstring(L, error.c_str());
		return 2;
	}
	
	lua_pushstring(L, outlua.c_str());
	return 1;
}

