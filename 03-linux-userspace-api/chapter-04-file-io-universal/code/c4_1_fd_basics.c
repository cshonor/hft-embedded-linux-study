/* c4_1_fd_basics.c — fd 到底是什么：进程内「打开文件表」的下标
 *
 * 演示四件事：
 *   1) 三个标准 fd（0/1/2）的常量值
 *   2) 新 fd 从「当前最小可用」数字分配
 *   3) close 掉的号立刻被回收复用
 *   4) fd 表在内核里的样子（/proc/self/fd/ 这个目录本身就是一个 fd）
 *
 * 编译: gcc -O0 -Wall -o c4_1_fd_basics c4_1_fd_basics.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

static void list_open_fds(const char *tag)
{
    DIR *d = opendir("/proc/self/fd");      /* 注意：opendir 自己也会占一个 fd */
    struct dirent *e;

    printf("  [%s] /proc/self/fd 里有: ", tag);
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.')            /* 跳过 . 和 .. */
            continue;
        printf("%s ", e->d_name);
    }
    printf("\n");
    closedir(d);
}

int main(void)
{
    printf("== 1. 三个标准 fd 的常量 ==\n");
    printf("  STDIN_FILENO  = %d\n", STDIN_FILENO);
    printf("  STDOUT_FILENO = %d\n", STDOUT_FILENO);
    printf("  STDERR_FILENO = %d\n", STDERR_FILENO);

    printf("\n== 2. 新 fd 从「最小可用」开始分配 ==\n");
    int a = open("/tmp/c4_a.txt", O_CREAT | O_RDWR, 0644);
    int b = open("/tmp/c4_b.txt", O_CREAT | O_RDWR, 0644);
    printf("  open a -> %d\n", a);
    printf("  open b -> %d\n", b);

    printf("\n== 3. close 之后那个号立刻被回收 ==\n");
    close(a);
    int c = open("/tmp/c4_c.txt", O_CREAT | O_RDWR, 0644);
    printf("  close(%d) 后 open c -> %d   （%s）\n", a, c,
           c == a ? "复用了刚释放的号" : "没有复用，说明另有空闲号");

    printf("\n== 4. fd 表长什么样：/proc/self/fd ==\n");
    list_open_fds("a b c 都开着时");
    printf("  当前我持有的: a=%d b=%d c=%d\n", a, b, c);

    printf("\n== 5. 经典技巧：关掉 0 号，新 fd 就会拿到 0 ==\n");
    int saved0 = dup(STDIN_FILENO);         /* 先把 0 号备份到别的号 */
    close(STDIN_FILENO);
    int d = open("/tmp/c4_d.txt", O_CREAT | O_RDWR, 0644);
    printf("  close(0) 后 open d -> %d   （%s）\n", d,
           d == 0 ? "拿到 0 号：0 不是特权，只是默认值" : "没拿到 0");

    dup2(saved0, STDIN_FILENO);             /* 恢复 0 号 */
    close(saved0);
    close(b);
    close(c);
    close(d);
    list_open_fds("收尾");
    return 0;
}
