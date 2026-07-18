CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0 -I../common

demo06_callback: app/main.c sensor/sensor.c
	$(CC) $(CFLAGS) -o $@ app/main.c sensor/sensor.c

clean:
	rm -f demo06_callback

.PHONY: clean
