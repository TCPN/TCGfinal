#!/bin/bash

LDFLAGS=-static -s -lwsock32
.PHONY:	clean
search.exe:	main.cc anqi.cc ClientSocket.cpp Protocol.cpp
	$(CXX) -o $@ $^ $(LDFLAGS)
clean:
	DEL search.exe 2>NUL
distr:
	$(foreach D,$(shell echo ../Room*/Search),cp search.exe $(D);)