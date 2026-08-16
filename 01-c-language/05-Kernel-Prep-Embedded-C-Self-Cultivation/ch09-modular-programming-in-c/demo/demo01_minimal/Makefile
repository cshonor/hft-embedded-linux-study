CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0
INC     := -I. -I../common -Idriver -Iutils

demo01_minimal: app/main.c driver/uart.c utils/crc.c ../common/err.c
	$(CC) $(CFLAGS) $(INC) -o $@ \
	    app/main.c driver/uart.c utils/crc.c ../common/err.c

clean:
	rm -f demo01_minimal

.PHONY: clean
