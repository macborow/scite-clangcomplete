
CC=gcc
CFLAGS=-shared -fpic -W -Wall -pedantic -O0 -ggdb
LDFLAGS=-L /usr/lib/llvm -llua -lclang



THEDLL=clangcomplete.so
OBJECTS=clangcomplete.o

$(THEDLL) : $(OBJECTS)
	$(CC) $(CFLAGS) -shared $^ -o $@ $(LDFLAGS)

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ 

clean :
	rm -rf $(THEDLL) $(OBJECTS)

.PHONY: clean


