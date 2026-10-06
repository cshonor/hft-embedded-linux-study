CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g

# 正常路径：声明/实现分离，全部参与链接
all: demo
demo: main.c math_ext.c log.c
	$(CC) $(CFLAGS) -o demo main.c math_ext.c log.c

# 情形 A：外部链接 + 无人调用，缺 log.o —— 任何编译器/优化级别都报错
link_fail_ext: math_ext.c main.c
	$(CC) $(CFLAGS) -O0 -c math_ext.c -o math_ext.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	-$(CC) math_ext.o main.o -o fail_ext
	nm math_ext.o | grep -E 'multiply|logmsg'

# 情形 B vs C：static + 无人调用，缺 log.o
# gcc -O0 报错（保留死函数）；gcc -O2 / clang 通过（消除死函数）
link_static_O0: math_static.c main.c
	$(CC) $(CFLAGS) -O0 -c math_static.c -o ms0.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	nm ms0.o | grep -E 'multiply|logmsg' || echo "(ms0.o: multiply 已被消除)"
	-$(CC) ms0.o main.o -o fail_s0 && echo "O0 link OK (编译器已消除)"

link_static_O2: math_static.c main.c
	$(CC) $(CFLAGS) -O2 -c math_static.c -o ms2.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	nm ms2.o | grep -E 'multiply|logmsg' || echo "(ms2.o: multiply 已被消除)"
	-$(CC) ms2.o main.o -o fail_s2 && echo "O2 link OK (编译器已消除)"

clean:
	rm -f demo fail_ext fail_s0 fail_s2 *.o
.PHONY: all clean link_fail_ext link_static_O0 link_static_O2
