/* c6_7_environ.c — 6.7 环境变量：getenv/setenv/unsetenv/environ + putenv 两个陷阱
 * 编译: gcc -O2 -Wall -Wextra -o c6_7_environ c6_7_environ.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

static void show(const char *name)
{
    char *v = getenv(name);
    printf("  getenv(\"%s\") = %s\n", name, v ? v : "(NULL)");
}

int main(void)
{
    printf("=== A. 只读查询 ===\n");
    show("PATH");
    show("NO_SUCH_VAR");

    printf("=== B. setenv / unsetenv（推荐路径）===\n");
    setenv("MY_VAR", "hello", 0);
    show("MY_VAR");
    setenv("MY_VAR", "world", 0);      /* overwrite=0 -> 不覆盖 */
    show("MY_VAR");
    setenv("MY_VAR", "world", 1);      /* overwrite=1 -> 覆盖 */
    show("MY_VAR");
    unsetenv("MY_VAR");
    show("MY_VAR");

    printf("=== C. putenv 陷阱一：传入的是局部数组（栈上）===\n");
    {
        char localbuf[32];
        strcpy(localbuf, "PITFALL=local-stack-buffer");
        putenv(localbuf);              /* 内核环境区现在指向这个栈数组 */
        printf("  函数返回前: %s\n", getenv("PITFALL"));
    }
    /* localbuf 已随作用域失效 —— 这里读它是 UB（演示只打印指针是否还在） */
    printf("  块已退出，getenv(\"PITFALL\") 仍能命中(悬垂): %s\n",
           getenv("PITFALL") ? "是（危险！）" : "否");

    printf("=== D. putenv 陷阱二：改的是调用者自己的缓冲 ===\n");
    static char same[] = "MUTABLE=aaa";
    putenv(same);
    show("MUTABLE");
    strcpy(same, "MUTABLE=bbb");       /* 直接改那块内存 -> 环境跟着变 */
    show("MUTABLE");

    printf("=== E. 遍历 environ ===\n");
    int n = 0;
    for (char **e = environ; *e; e++) n++;
    printf("  环境变量共 %d 条，前 3 条:\n", n);
    for (int i = 0; i < 3 && environ[i]; i++)
        printf("    %s\n", environ[i]);
    return 0;
}
