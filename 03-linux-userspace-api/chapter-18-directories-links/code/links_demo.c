/* Hard link vs symlink: compare inode / nlink via lstat.
 * cc -Wall -Wextra -o links_demo links_demo.c
 * ./links_demo [/tmp/tlpi_links_demo]
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void show(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror(path);
        return;
    }
    printf("%-40s ino=%llu nlink=%lu mode=%s\n",
           path,
           (unsigned long long)st.st_ino,
           (unsigned long)st.st_nlink,
           S_ISLNK(st.st_mode) ? "symlink" :
           S_ISREG(st.st_mode) ? "reg" : "other");
}

int main(int argc, char *argv[])
{
    const char *base = (argc > 1) ? argv[1] : "/tmp/tlpi_links_demo";
    char orig[256], hard[256], soft[256];
    int fd;

    snprintf(orig, sizeof(orig), "%s.orig", base);
    snprintf(hard, sizeof(hard), "%s.hard", base);
    snprintf(soft, sizeof(soft), "%s.soft", base);

    unlink(orig);
    unlink(hard);
    unlink(soft);

    fd = open(orig, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    if (write(fd, "data\n", 5) != 5) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    if (link(orig, hard) == -1) {
        perror("link");
        return EXIT_FAILURE;
    }
    if (symlink(orig, soft) == -1) {
        perror("symlink");
        return EXIT_FAILURE;
    }

    printf("hard link shares inode; symlink has its own:\n");
    show(orig);
    show(hard);
    show(soft);

    {
        char buf[256];
        ssize_t n = readlink(soft, buf, sizeof(buf) - 1);
        if (n >= 0) {
            buf[n] = '\0';
            printf("readlink(%s) = \"%s\"\n", soft, buf);
        }
    }

    unlink(soft);
    unlink(hard);
    unlink(orig);
    return 0;
}
