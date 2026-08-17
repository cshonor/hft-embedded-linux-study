CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -I../part-a-alu-host
TARGET   = cpu_sim
SRCS     = cpu.c main.c ../part-a-alu-host/alu.c

.PHONY: all test run trace clean

all: $(TARGET)

$(TARGET): $(SRCS) cpu.h isa.h ../part-a-alu-host/alu.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

test: $(TARGET)
	./$(TARGET)

run: test

trace: $(TARGET)
	./$(TARGET) --trace sum

clean:
	rm -f $(TARGET)
