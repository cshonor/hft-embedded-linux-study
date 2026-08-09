CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = echo_server

.PHONY: all clean run

all: $(TARGET)

$(TARGET): echo_server.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
