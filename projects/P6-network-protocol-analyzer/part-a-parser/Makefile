CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = parse_stub

.PHONY: all clean run

all: $(TARGET)

$(TARGET): parse_stub.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
