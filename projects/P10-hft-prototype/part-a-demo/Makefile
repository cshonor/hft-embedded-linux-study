CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
SRC       = src/main.cpp
TARGET    = hft_demo

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): $(SRC) src/*.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

test: $(TARGET)
	./$(TARGET) --self-test

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
