CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g
TARGET   = poll_fwd

.PHONY: all test clean

all: $(TARGET)

$(TARGET): poll_fwd.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
