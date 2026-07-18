CXX     ?= g++
CXXFLAGS ?= -Wall -Wextra -std=c++17 -g

all: demo

demo: main.cpp
	$(CXX) $(CXXFLAGS) -o demo main.cpp

clean:
	rm -f demo *.o

.PHONY: all clean
