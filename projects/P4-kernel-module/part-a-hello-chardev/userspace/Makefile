CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = hello_user

.PHONY: all clean

all: $(TARGET)

$(TARGET): hello_user.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
