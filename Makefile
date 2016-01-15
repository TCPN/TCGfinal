#!/bin/bash

LDFLAGS=-static -s -lwsock32 -D_WINDOWS
.PHONY:	clean
search.exe:	main.cc anqi.cc ClientSocket.cpp Protocol.cpp
	$(CXX) -o $@ $^ $(LDFLAGS)
clean:
	DEL search.exe 2>NUL
distr: search.exe
	$(foreach D,$(shell echo ../Room*/Search),cp search.exe $(D);ls -l $(D)/search.exe;)
toC: search.exe
	cp search.exe ../RoomCreate/Search/search.exe
	ls -l ../RoomCreate/Search
toE: search.exe
	cp search.exe ../RoomEnter/Search/search.exe
	ls -l ../RoomEnter/Search