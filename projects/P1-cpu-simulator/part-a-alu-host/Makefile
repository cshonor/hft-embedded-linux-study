CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g
TARGET   = alu_test

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): alu.c alu_test.c alu.h
	$(CC) $(CFLAGS) -o $@ alu.c alu_test.c

test: $(TARGET)
	./$(TARGET)

run: test

clean:
	rm -f $(TARGET)
