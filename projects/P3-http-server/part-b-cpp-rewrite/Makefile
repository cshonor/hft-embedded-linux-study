CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g
TARGET = echo_server

.PHONY: all clean run

all: $(TARGET)

$(TARGET): echo_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
