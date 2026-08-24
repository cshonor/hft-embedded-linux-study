CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -std=c++17 -g
TARGET    = lob_test

.PHONY: all test clean

all: $(TARGET)

$(TARGET): lob_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
