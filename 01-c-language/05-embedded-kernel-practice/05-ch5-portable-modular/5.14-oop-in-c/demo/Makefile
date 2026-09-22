CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=gnu11 -g -O0

BINS = demo01_encapsulation demo02_inherit demo03_polymorphism \
       demo04_lifecycle demo05_multi_iface demo06_layered

all: $(BINS)

demo01_encapsulation: demo01_main.c counter.c counter.h
	$(CC) $(CFLAGS) -o $@ demo01_main.c counter.c

demo02_inherit: demo02_main.c uart_dev.c device.h
	$(CC) $(CFLAGS) -o $@ demo02_main.c uart_dev.c

demo03_polymorphism: demo03_main.c dev_ops.c dev_ops.h
	$(CC) $(CFLAGS) -o $@ demo03_main.c dev_ops.c

demo04_lifecycle: demo04_main.c dev_ops.c dev_ops.h
	$(CC) $(CFLAGS) -o $@ demo04_main.c dev_ops.c

demo05_multi_iface: demo05_multi_iface.c
	$(CC) $(CFLAGS) -o $@ $<

demo06_layered:
	$(CC) $(CFLAGS) -I layered/abstract -o $@ \
	    layered/abstract/device.c \
	    layered/hw/uart.c \
	    layered/app/app.c \
	    layered/app/main.c

clean:
	rm -f $(BINS)

.PHONY: all clean
