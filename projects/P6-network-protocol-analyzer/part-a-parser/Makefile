CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g
TARGET   = parse

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): parse.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

run: test

clean:
	rm -f $(TARGET)
