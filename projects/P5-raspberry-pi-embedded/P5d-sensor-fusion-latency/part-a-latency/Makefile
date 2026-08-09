CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pthread
TARGET = latency_stub

.PHONY: all clean run

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
