/* 同进程两次 open 同一文件 → 两个 struct file → 独立偏移
 * 编译: cc -Wall -o dual_open_fd_offset_demo dual_open_fd_offset_demo.c
 * 运行: ./dual_open_fd_offset_demo
 *
 * 预期: fd1 读走前 3 字节后，fd2 仍从文件头读到 "123"。
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    const char *path = "/tmp/dual_open_fd_offset_demo.txt";
    const char *payload = "123456";
    int fd1, fd2;
    char buf[8];
    ssize_t n;

    fd1 = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd1 < 0) {
        perror("open fd1");
        return 1;
    }
    if (write(fd1, payload, strlen(payload)) < 0) {
        perror("write");
        return 1;
    }
    if (lseek(fd1, 0, SEEK_SET) < 0) {
        perror("lseek fd1");
        return 1;
    }

    fd2 = open(path, O_RDONLY);
    if (fd2 < 0) {
        perror("open fd2");
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    n = read(fd1, buf, 3);
    printf("fd1=%d read %zd: \"%s\" (this session offset now 3)\n", fd1, n, buf);

    memset(buf, 0, sizeof(buf));
    n = read(fd2, buf, 3);
    printf("fd2=%d read %zd: \"%s\" (independent session, still from start)\n",
           fd2, n, buf);

    close(fd1);
    close(fd2);
    return 0;
}
