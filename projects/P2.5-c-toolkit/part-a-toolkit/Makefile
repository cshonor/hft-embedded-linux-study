CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g
TARGET   = test_toolkit
DEMO     = layout_demo

.PHONY: all test run clean

all: $(TARGET) $(DEMO)

$(TARGET): test_toolkit.c container_of.h list.h ringbuf.h compile_macros.h
	$(CC) $(CFLAGS) -o $@ test_toolkit.c

$(DEMO): layout_demo.c container_of.h
	$(CC) $(CFLAGS) -o $@ layout_demo.c

test: $(TARGET)
	./$(TARGET)

run: test $(DEMO)
	./$(DEMO)

clean:
	rm -f $(TARGET) $(DEMO)
