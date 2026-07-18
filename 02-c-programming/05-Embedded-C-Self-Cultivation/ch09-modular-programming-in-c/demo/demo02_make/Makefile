CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0
INC     := -I../common -I. -Idriver -Iutils

.PHONY: all clean mod_uart mod_utils install

all: demo02_app

../common/libcommon.a:
	$(MAKE) -C ../common

utils/libutils.a: utils/crc.c utils/crc.h
	$(CC) $(CFLAGS) $(INC) -c utils/crc.c -o utils/crc.o
	$(AR) cr $@ utils/crc.o

driver/libuart.a: driver/uart.c driver/uart.h ../common/libcommon.a
	$(CC) $(CFLAGS) $(INC) -c driver/uart.c -o driver/uart.o
	$(AR) cr $@ driver/uart.o

demo02_app: app/main.c utils/libutils.a driver/libuart.a ../common/libcommon.a
	$(CC) $(CFLAGS) $(INC) -o $@ app/main.c \
	    utils/libutils.a driver/libuart.a ../common/libcommon.a

mod_uart: driver/libuart.a
mod_utils: utils/libutils.a

install: all
	mkdir -p ../out/lib ../out/include
	cp driver/libuart.a utils/libutils.a ../common/libcommon.a ../out/lib/
	cp driver/uart.h utils/crc.h ../common/err.h ../common/log.h ../out/include/

clean:
	rm -f demo02_app utils/*.o utils/libutils.a driver/*.o driver/libuart.a
	$(MAKE) -C ../common clean
