#include <stdio.h>

int main(void)
{
    const char *p1 = "hello .rodata";       /* 内容只读，指针可变 */
    char buf[] = "mutable on stack";
    char *const p2 = buf;                   /* 指针只读，内容可写 */
    const char *const p3 = "both readonly";

    printf("p1 -> %s\n", p1);
    p2[0] = 'M';
    printf("p2 -> %s (stack buf, content writable)\n", p2);

    /* 取消注释会触发段错误或编译错误：
     * p1[0] = 'H';   // SIGSEGV — rodata
     * p2 = buf + 1;  // 编译错误 — p2 是 char* const
     * p3 = "x";      // 编译错误
     * p3[0] = 'H';   // 编译错误
     */
    (void)p3;
    return 0;
}
