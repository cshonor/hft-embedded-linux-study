CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = alu_stub

.PHONY: all clean run

all: $(TARGET)

$(TARGET): alu_stub.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
