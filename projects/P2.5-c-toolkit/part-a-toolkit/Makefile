CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -g
TARGET = layout_demo

.PHONY: all clean run

all: $(TARGET)

$(TARGET): layout_demo.c container_of.h
	$(CC) $(CFLAGS) -o $@ layout_demo.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
