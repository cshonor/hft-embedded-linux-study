/* fork 后父子共享同一 struct file → 共享文件偏移
 * 编译: cc -Wall -o fork_fd_offset_demo fork_fd_offset_demo.c
 * 运行: ./fork_fd_offset_demo
 *
 * 预期: 父读走前 3 字节后，子再读不会从头，而是接着 offset。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void)
{
    const char *path = "/tmp/fork_fd_offset_demo.txt";
    const char *payload = "123456";
    int fd;
    char buf[8];
    ssize_t n;
    pid_t pid;

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    if (write(fd, payload, strlen(payload)) < 0) {
        perror("write");
        return 1;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        return 1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* 子: 稍等父先读，再读同一 fd */
        usleep(100000);
        memset(buf, 0, sizeof(buf));
        n = read(fd, buf, 3);
        printf("child  pid=%d read %zd bytes: \"%s\" (continues shared offset)\n",
               getpid(), n, buf);
        close(fd);
        _exit(0);
    }

    /* 父: 先读 3 字节，把共享 struct file 的 offset 推到 3 */
    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, 3);
    printf("parent pid=%d read %zd bytes: \"%s\" (offset now 3)\n",
           getpid(), n, buf);
    waitpid(pid, NULL, 0);
    close(fd);
    return 0;
}
