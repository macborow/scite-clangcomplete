LUA_INCLUDE=../scite/lua/include
CFLAGS=-Os -pedantic 
LIBS=-lscite -llibclang.dll
LIBDIR= -L../scite/lualib/lib/ -L../llvm/build/lib
INCLUDE = -I$(LUA_INCLUDE) -I../llvm/tools/clang/include

OBJS=clangcomplete.o 

clangcomplete.dll: $(OBJS)
	g++ $(CFLAGS) -s -shared -o clangcomplete.dll $(OBJS) $(LIBDIR) $(LIBS)

clangcomplete.o: clangcomplete.cpp
	g++ $(CFLAGS) -c $(INCLUDE) clangcomplete.cpp

clean:
	rm *.o

.cpp.o:
	g++ $(CFLAGS) -c $< -o $@
