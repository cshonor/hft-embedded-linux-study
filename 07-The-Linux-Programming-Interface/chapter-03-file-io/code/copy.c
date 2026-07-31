/* TLPI Ch4 Listing 4-1 精神：通用 I/O 模型文件拷贝
 * 编译: cc -Wall -o copy copy.c
 * 用法: ./copy src dst
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    int inputFd, outputFd;
    ssize_t numRead;
    char buf[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s src dst\n", argv[0]);
        return 1;
    }

    inputFd = open(argv[1], O_RDONLY);
    if (inputFd < 0) {
        perror("open src");
        return 1;
    }

    outputFd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                    S_IROTH | S_IWOTH);
    if (outputFd < 0) {
        perror("open dst");
        return 1;
    }

    while ((numRead = read(inputFd, buf, BUF_SIZE)) > 0) {
        ssize_t numWritten = write(outputFd, buf, (size_t)numRead);
        if (numWritten != numRead) {
            fprintf(stderr, "write partial or error\n");
            return 1;
        }
    }
    if (numRead == -1) {
        perror("read");
        return 1;
    }

    if (close(inputFd) == -1 || close(outputFd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
