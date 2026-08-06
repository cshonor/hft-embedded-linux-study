CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -Wall -Wextra -std=c11 -g
CXXFLAGS ?= -Wall -Wextra -std=c++17 -g

all: demo

demo: c_add.o main.o
	$(CXX) $(CXXFLAGS) -o demo c_add.o main.o

c_add.o: c_add.c
	$(CC) $(CFLAGS) -c c_add.c -o c_add.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

clean:
	rm -f *.o demo

.PHONY: all clean
