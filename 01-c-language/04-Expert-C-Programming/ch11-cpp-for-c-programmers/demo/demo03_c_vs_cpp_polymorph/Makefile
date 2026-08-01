CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -Wall -Wextra -std=c11 -g
CXXFLAGS ?= -Wall -Wextra -std=c++17 -g

all: c_demo cpp_demo

c_demo: c_ops.c
	$(CC) $(CFLAGS) -o c_demo c_ops.c

cpp_demo: cpp_virtual.cpp
	$(CXX) $(CXXFLAGS) -o cpp_demo cpp_virtual.cpp

clean:
	rm -f c_demo cpp_demo *.o

.PHONY: all clean
