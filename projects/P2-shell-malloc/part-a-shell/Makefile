CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
SRCS = shell.c parser.c executor.c builtin.c
OBJS = $(SRCS:.c=.o)
TARGET = myshell

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
