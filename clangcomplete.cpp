#include <string>
#include <cstring>
#include <fstream>

#include <clang-c/Index.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

int clangAutocomplete(lua_State* L, char *sourceFilePath, char *sourceBuffer, unsigned long sourceBufferLength,
                      int line, int column, char **args, size_t numArgs, const char *prefix);
int completeGetLuaArgs(lua_State* L, char* &sourceFilePath, char* &sourceBuffer, unsigned long &sourceBufferLength,
                       unsigned &offset, char** &args, size_t &numArgs);
void convertOffsetToLineAndColumn(char* sourceBuffer, unsigned offset, int &line, int &column);

// usage: complete(string SourceFilePath, int CurrentOffsetWithinFile, string SourceFileUnsavedBuffer, table compilerOptionsAsStrings)
int completeGetLuaArgs(lua_State* L, char* &sourceFilePath, char* &sourceBuffer, unsigned long &sourceBufferLength, unsigned &offset, char** &args, size_t &numArgs)
{
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TNUMBER);
    luaL_checktype(L, 3, LUA_TSTRING);
    luaL_checktype(L, 4, LUA_TTABLE);

    // 4th param: list of compiler options, i.e. include paths, preprocessor definitions, etc.
    {
        numArgs = lua_objlen(L, 4);
        args = new char*[numArgs];
        size_t len = 0;
        for(unsigned int i = 1; i <= numArgs; ++i)  // lua params are indexed from 1
        {
            lua_rawgeti(L, 4, i);
            const char *param = (char*)luaL_checklstring(L, -1, &len);
            *(args + i - 1) = new char[len+1];
            strncpy(*(args + i - 1), param, len);
            (*(args + i - 1))[len] = 0;
            lua_pop(L,1);
        }
    }
    lua_pop(L,1);
    
    //3rd param: buffer contents (unsaved contents of the source file)
    {
        size_t len;
        const char *param = lua_tolstring(L, -1, &len);
        sourceBufferLength = len;
        sourceBuffer = new char[len+1];
        strncpy(sourceBuffer, param, len);
        sourceBuffer[len] = 0;
    }
	lua_pop(L,1);

    //2nd param: offset in the file where the cursor is (i.e. where code completion takes place)
    offset = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // 1st param: source file path
    {
        size_t len;
        const char *param = lua_tolstring(L, -1, &len);
        sourceFilePath = new char[len+1];
        strncpy(sourceFilePath, param, len);
        sourceFilePath[len] = 0;
    }
	lua_pop(L,1);
    
    if (offset>sourceBufferLength)
    {
        lua_pushstring(L, "complete: offset (param #2) is larger than the buffer (param #3)");
        lua_error(L);
    }
    

}

void convertOffsetToLineAndColumn(char* sourceBuffer, unsigned offset, int &line, int &column)
{
    // convert the offset to row/column
    line = column = 1;
    for (char *pos=sourceBuffer, *bufferEnd=(sourceBuffer + offset); pos!=bufferEnd; ++pos) {
        if (*pos == '\n')
        {
            ++line;
            column=1;
        } else if (*pos != '\r') {
            ++column;
        }
    }    
}

int do_complete(lua_State* L)
{
    char *sourceFilePath=0, *sourceBuffer=0;
    unsigned long sourceBufferLength=0;
    int line=0, column=0;
    unsigned offset=0;
    char** args=0;
    size_t numArgs=0;
    
    completeGetLuaArgs(L, sourceFilePath, sourceBuffer, sourceBufferLength, offset, args, numArgs);
    convertOffsetToLineAndColumn(sourceBuffer, offset, line, column);
    
    return clangAutocomplete(L, sourceFilePath, sourceBuffer, sourceBufferLength, line, column, args, numArgs, NULL);
}

int clangAutocomplete(lua_State* L, char *sourceFilePath, char *sourceBuffer, unsigned long sourceBufferLength, int line, int column, char **args, size_t numArgs, const char *prefix)
{
    // excludeDeclsFromPCH = 1, displayDiagnostics=1
    CXIndex idx = clang_createIndex(1, 1);
    
    CXUnsavedFile unsavedFile;
    unsavedFile.Filename = sourceFilePath;
    unsavedFile.Contents = sourceBuffer;
    unsavedFile.Length = sourceBufferLength;
    
    CXTranslationUnit tu = clang_parseTranslationUnit(
        idx, // CXIndex
        sourceFilePath,
        args, // command_line_args,
        numArgs,
        &unsavedFile,
        1, // num_unsaved_files,
        CXTranslationUnit_None //~ or possibly: CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_CacheCompletionResults | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
        );
 CXCodeCompleteResults *ccResults = clang_codeCompleteAt(
            tu,    // CXTranslationUnit
            sourceFilePath,
            line,
            column,
            &unsavedFile,
            1, // num_unsaved_files,
            clang_defaultCodeCompleteOptions()
            );
    
    if (!ccResults)
    {
        lua_pushstring(L, "complete: error - make sure all parameters have correct type");
        lua_error(L);
    }
        
    // sort completion results in alphabetical order
    clang_sortCodeCompletionResults(ccResults->Results, ccResults->NumResults);

    // if filtering results to match prefix - calculate the size of the result table
    int numResults = ccResults->NumResults;
    if ((prefix != NULL) or (strlen(prefix) > 0))
    {
        numResults = 0;
        int prefixLen = strlen(prefix);
        CXCompletionResult* ccResult = ccResults->Results;
        for (int i=0; i<ccResults->NumResults; ++i)
        {
            int numChunks = clang_getNumCompletionChunks (ccResult->CompletionString);
            for (int chunkIdx=0; chunkIdx < numChunks; ++chunkIdx)
            {
                CXCompletionChunkKind kind = clang_getCompletionChunkKind (ccResult->CompletionString, chunkIdx);
                if (kind == CXCompletionChunk_TypedText)
                {
                    const char* completionString = (const char*)(clang_getCompletionChunkText (ccResult->CompletionString, chunkIdx).data);
                    bool matches = true;
                    for (int pos = 0; pos < prefixLen; ++pos)
                    {
                        if (*(completionString + pos) != *(prefix + pos))
                        {
                            matches = false;
                            break;
                        }
                    }
                    if (matches)
                    {
                        ++numResults;
                    }
                    break;
                }
            }
            ++ccResult;
        }
    }
    
    // create a lua table to hold the results
    lua_createtable(L, numResults, 0);
    int ccResultsTable = lua_gettop(L);
    
    CXCompletionResult* ccResult = ccResults->Results;
    int resultTableIdx = 0;
    for (int i=0; i < ccResults->NumResults; ++i)
    {
        int numChunks = clang_getNumCompletionChunks (ccResult->CompletionString);
        
        // check whether the result should be included (only if matches the existing prefix)
        bool matches = true;
        if (prefix != NULL)
        {
            int prefixLen = strlen(prefix);
            for (int chunkIdx=0; chunkIdx < numChunks; ++chunkIdx)
            {
                CXCompletionChunkKind kind = clang_getCompletionChunkKind (ccResult->CompletionString, chunkIdx);
                if (kind == CXCompletionChunk_TypedText)
                {
                    const char* completionString = (const char*)(clang_getCompletionChunkText (ccResult->CompletionString, chunkIdx).data);
                    for (int pos = 0; pos < prefixLen; ++pos)
                    {
                        if (*(completionString + pos) != *(prefix + pos))
                        {
                            matches = false;
                            break;
                        }
                    }
                    break;
                }
            }
        }
        if (matches)
        {
            ++resultTableIdx;
            // create a table for chunks
            lua_createtable(L, 1, 0);
            int chunksTable = lua_gettop(L);
            
            for (int chunkIdx=0; chunkIdx < numChunks; ++chunkIdx)
            {
                
                CXCompletionChunkKind kind = clang_getCompletionChunkKind (ccResult->CompletionString, chunkIdx);
                
                lua_createtable(L, 1, 0);
                int singleChunkTable = lua_gettop(L);
                
                lua_pushinteger(L, (int)kind);
                lua_rawseti(L, singleChunkTable, 1);
                lua_pushstring(L, (const char*)(clang_getCompletionChunkText (ccResult->CompletionString, chunkIdx).data));
                lua_rawseti(L, singleChunkTable, 2);
                lua_rawseti(L, chunksTable, chunkIdx+1);
            }
            lua_rawseti(L, ccResultsTable, resultTableIdx);
        }
        ++ccResult;
    }
    
    clang_disposeCodeCompleteResults(ccResults);
    clang_disposeIndex(idx);
    clang_disposeTranslationUnit(tu);
    
    delete[] sourceFilePath;
    delete[] sourceBuffer;
    
    if (prefix != NULL) {
        lua_pushstring(L, prefix);  // return the prefix as a second value
        return 2;
    }
    
    return 1;
}

int do_completeSymbol(lua_State *L)
{
    char *sourceFilePath=0, *sourceBuffer=0;
    unsigned long sourceBufferLength=0;
    int line=0, column=0;
    char** args=0;
    unsigned offset=0;
    size_t numArgs=0;
    
    completeGetLuaArgs(L, sourceFilePath, sourceBuffer, sourceBufferLength, offset, args, numArgs);
    
    // remove chars until the beginning of current token and store the prefix that has already been typed
    unsigned newOffset = offset;
    char *prefix = 0;
    {
        char *currentCharPtr = sourceBuffer + newOffset - 1;
        char currentChar = 0;
        while (true)
        {
            if (newOffset == 0) break;
            currentChar = *currentCharPtr;
            if (currentChar == '\n') break;  // read only to the beginning of current line
            if (!(((currentChar >= '0') && (currentChar <= '9')) || ((currentChar >= 'A') && (currentChar <= 'Z')) || ((currentChar >= 'a') && (currentChar <= 'z'))))
            {
                break;
            }
            --currentCharPtr;
            --newOffset;
        }
        const int prefixLength = offset - newOffset;
        prefix = new char[prefixLength + 1];
        prefix[prefixLength] = 0;
        for (int i = 0; i < prefixLength; ++i)
        {
            prefix[i] = *(++currentCharPtr);
            *currentCharPtr = ' ';
        }
    }
    
    convertOffsetToLineAndColumn(sourceBuffer, newOffset, line, column);
    
    return clangAutocomplete(L, sourceFilePath, sourceBuffer, sourceBufferLength, line, column, args, numArgs, prefix);
}

static const struct luaL_reg api[] = {
    {"complete",do_complete},
    {"completeSymbol",do_completeSymbol}, // works within symbols
    {NULL, NULL},
};

extern "C" __declspec(dllexport)
int luaopen_clangcomplete(lua_State *L)
{
    luaL_openlib (L, "clangcomplete", api, 0);
    return 1;
}