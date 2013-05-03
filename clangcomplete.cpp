/* vim: set ts=8 sw=8 noexpandtab: */

#include <string.h>

#include <clang-c/Index.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#ifdef PLATFORM_MINGW
#define __DLLEXPORT__ __declspec(dllexport)
#else
#define __DLLEXPORT__ 
#endif


#define CXCPLRES_META	"cxcplres"


static inline int make_lua_table(lua_State *L)
{
	lua_newtable(L);
	return lua_gettop(L);
}


static int cplresult_push(lua_State *L, CXCodeCompleteResults *res)
{
	CXCodeCompleteResults **p = static_cast<CXCodeCompleteResults **>(lua_newuserdata(L, sizeof(res)));
	luaL_getmetatable(L, CXCPLRES_META);
	lua_setmetatable(L, -2);  
	*p = res;
	return 1;
}


static int cplresult_gc(lua_State *L)
{
	CXCodeCompleteResults **p = static_cast<CXCodeCompleteResults **>(luaL_checkudata(L, 1, CXCPLRES_META));
	if (*p != NULL) {
   		clang_disposeCodeCompleteResults(*p);
		*p = NULL;
	}
	
	return 0;
}


static int cplresult_kind(lua_State *L)
{
	CXCodeCompleteResults **p = static_cast<CXCodeCompleteResults **>(luaL_checkudata(L, 1, CXCPLRES_META));
	unsigned int n = luaL_checkinteger(L, 1);

	if (n < (*p)->NumResults) {
		CXCompletionResult *res = (*p)->Results + n;
		lua_pushinteger(L, res->CursorKind);
		return 1;
	}

	return 0;
}


static int cplresult_chunks(lua_State *L)
{
	CXCodeCompleteResults **p = static_cast<CXCodeCompleteResults **>(luaL_checkudata(L, 1, CXCPLRES_META));
	unsigned int n = luaL_checkinteger(L, 1);

	if (n < (*p)->NumResults) {
		CXCompletionResult *res = (*p)->Results + n;
		int tab = make_lua_table(L);
		int count = clang_getNumCompletionChunks(res->CompletionString);
		int i;

		for (i = 0; i < count; ++i) {
			lua_pushinteger(L, i + 1);
			lua_pushstring(L, clang_getCString(clang_getCompletionChunkText(res->CompletionString, i)));
			lua_settable(L, tab);
		}

		return 1;
	}

	return 0;
}




static const struct luaL_Reg cplresult_metaops[] = {
	{ "__gc",    cplresult_gc },
	{ "release", cplresult_gc },
	{ "kind",    cplresult_kind },
	{ "chunks",  cplresult_chunks },
	{ NULL, NULL }
};



static CXCodeCompleteResults *clang_docomplete(const char *path, const char *content,
        int line, int col, int clang_argc, const char **clang_argv)
{
    CXCodeCompleteResults *res;
    struct CXUnsavedFile unsavedFile;
    CXTranslationUnit tu;
    CXIndex idx;

    
    idx = clang_createIndex(1, 1);

    unsavedFile.Filename = path;
    unsavedFile.Contents = content;
    unsavedFile.Length = strlen(content);
    
    tu = clang_parseTranslationUnit(idx, path, clang_argv, clang_argc, 
		    &unsavedFile, 1, CXTranslationUnit_None);

    res = clang_codeCompleteAt(tu, path, line, col, &unsavedFile, 1, 
            clang_defaultCodeCompleteOptions());

    clang_disposeIndex(idx);
    clang_disposeTranslationUnit(tu);
    
    return res;
}


static int extract_clang_argv(const char **out, int max, lua_State *L, int idx)
{
	int total = 0;

	if (! lua_istable(L, idx))
		return 0;
	
	lua_pushnil(L);

	while (max--) {
		if (lua_next(L, idx) == 0)
			break;
		out[total++] = lua_tostring(L, -1);
		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	return total;
}


static int do_complete(lua_State *L)
{
	const char *clang_argv[512];
	const char *path = luaL_checkstring(L, 1);
	const char *content = luaL_checkstring(L, 2);
	int line = luaL_checkinteger(L, 3);
	int col = luaL_checkinteger(L, 4);
	int clang_argc = extract_clang_argv(clang_argv, 512, L, 5);

	CXCodeCompleteResults *res = clang_docomplete(path, content, line, col,
		       	clang_argc, clang_argv);

	return res == NULL ? 0 : cplresult_push(L, res);

}


static const struct luaL_Reg api[] = {
	{"complete",do_complete},
	{NULL, NULL}
};


extern "C" __DLLEXPORT__
int luaopen_clangcomplete(lua_State *L)
{
	luaL_newmetatable(L, CXCPLRES_META);
	luaL_register(L, NULL, cplresult_metaops);
	
	// meta[__metatable] = meta
	lua_pushliteral(L, "__metatable");                                                                                   
	lua_pushvalue(L, -2);                                                                                                
	
	// take off the metatable
	lua_pop(L, 1);

	luaL_register(L, "clangcomplete", api);
	return 1;
}
