/* c25_fork_stdio_three_fixes.c

   ⚠️ 延伸 demo —— **原书没有这个程序**。

   原书 Listing 25-2（`fork_stdio_buf.c`）只演示了**病**：
     "Hello world\n" 走 printf（进用户态缓冲）、然后 write("Ciao\n")、
     然后 fork()、然后父子都 exit() —— 当 stdout 是**块缓冲**（被重定向到文件）时，
     父进程那一刻还没 flush 的缓冲被 fork() 复制进子进程，
     于是父子各 flush 一次 ⇒ "Hello world" 打印两遍，且 "Ciao" 反而在前。

   原书 §25.4 接着给了**药**，一共三种：
     A. fork() 之前用 fflush()（或 setvbuf()/setbuf() 关掉缓冲）
     B. 子进程改用 _exit()（不 flush，也不跑 exit handler）
        —— 由此引出更一般的准则：一个程序里应该只有**一个**进程用 exit() 收场，
           通常是父进程
     C. （同 A 的后半）直接对 stdio 流关缓冲

   本程序把病与三种药放进同一个源文件，用 argv[1] 选模式，方便逐个对照。

   ⚠️ 在 CE 上跑：stdout 不是终端（是 socket）⇒ glibc 对 stdout 用**全缓冲**，
   所以 CE 上「模式 0」直接就等于原书里 `./fork_stdio_buf > a` 那一半，
   而原书 `./fork_stdio_buf`（终端，行缓冲）那一半**在 CE 上观察不到** —— 
   模式 3（关缓冲）是 CE 上唯一能拿到「类终端顺序」的办法。

   编译: gcc -O0 -Wall -Wextra -o c25_fork_stdio_three_fixes c25_fork_stdio_three_fixes.c
   运行: ./c25_fork_stdio_three_fixes <0|1|2|3>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void
label(int mode)
{
    const char *desc;
    char buf[256];

    switch (mode) {
    case 1:  desc = "修法 A：fork() 之前 fflush(stdout)";      break;
    case 2:  desc = "修法 B：子进程改用 _exit()";               break;
    case 3:  desc = "修法 C：setbuf(stdout, NULL) 关掉缓冲";    break;
    default: desc = "模式 0（原书 Listing 25-2 原样）";          break;
    }
    snprintf(buf, sizeof(buf), "--- 模式 %d | %s ---\n", mode, desc);
    write(STDOUT_FILENO, buf, strlen(buf));   /* 用 write 打标签，不参与缓冲顺序 */
}

int
main(int argc, char *argv[])
{
    int mode = (argc > 1) ? atoi(argv[1]) : 0;
    pid_t pid;

    /* 修法 C 必须在第一次 printf 之前生效 */
    if (mode == 3)
        setbuf(stdout, NULL);

    label(mode);

    printf("Hello world\n");                    /* → 用户态缓冲（模式 3 除外） */
    write(STDOUT_FILENO, "Ciao\n", 5);          /* → 立刻进内核缓冲 */

    if (mode == 1)
        fflush(stdout);                         /* 修法 A：把缓冲清空后再 fork */

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        if (mode == 2)
            _exit(EXIT_SUCCESS);                /* 修法 B：子进程不 flush */
        exit(EXIT_SUCCESS);
    }

    if (wait(NULL) == -1) {
        perror("wait");
        return EXIT_FAILURE;
    }
    exit(EXIT_SUCCESS);
}
