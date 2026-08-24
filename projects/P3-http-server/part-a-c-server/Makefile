CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g
TARGET   = echo_server

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): echo_server.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET) --self-test

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
