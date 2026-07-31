/* 经 libc write() 封装发起 write 系统调用
 * 编译: cc -Wall -o write_libc_demo write_libc_demo.c
 * 运行: ./write_libc_demo
 * 观察: strace -e write ./write_libc_demo
 *
 * 要点: C 里的 write() 是 libc 函数；跨界靠内部的 syscall 指令。
 */
#include <unistd.h>

int main(void)
{
    write(1, "hi via libc\n", 12);
    return 0;
}
