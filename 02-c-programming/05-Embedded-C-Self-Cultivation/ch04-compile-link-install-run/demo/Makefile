CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g -O0

all: demo01 demo_static demo_shared

# Demo01：四阶段产物
demo01: demo01_main.c
	$(CC) $(CFLAGS) -E demo01_main.c -o demo01.i
	$(CC) $(CFLAGS) -S demo01.i -o demo01.s
	$(CC) $(CFLAGS) -c demo01.s -o demo01.o
	$(CC) $(CFLAGS) demo01.o -o demo01

# Demo03：静态库
libdemo.a: func.o
	ar cr $@ func.o

demo_static: demo_lib_main.o libdemo.a
	$(CC) $(CFLAGS) -o $@ demo_lib_main.o -L. -ldemo

# Demo04：动态库 PIC
libdemo.so: func.o
	$(CC) -fPIC -shared func.o -o $@

demo_shared: demo_lib_main.o libdemo.so
	$(CC) $(CFLAGS) -o $@ demo_lib_main.o -L. -ldemo -Wl,-rpath,'$$ORIGIN'

func.o: func.c func.h
	$(CC) $(CFLAGS) -c func.c -o func.o

demo_lib_main.o: demo_lib_main.c func.h
	$(CC) $(CFLAGS) -c demo_lib_main.c -o demo_lib_main.o

clean:
	rm -f *.o *.i *.s *.a *.so demo01 demo_static demo_shared

.PHONY: all clean
