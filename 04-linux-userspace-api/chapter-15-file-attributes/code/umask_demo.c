/* Show umask clipping create mode.
 * cc -Wall -Wextra -o umask_demo umask_demo.c && ./umask_demo
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void create_show(const char *path, mode_t create_mode, mode_t mask)
{
    struct stat st;
    mode_t old;
    int fd;

    unlink(path);
    old = umask(mask);
    fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, create_mode);
    umask(old);
    if (fd == -1) {
        perror(path);
        return;
    }
    close(fd);

    if (stat(path, &st) == -1) {
        perror("stat");
        return;
    }
    printf("create_mode=%04o umask=%03o -> final=%04o  (expect %04o)\n",
           (unsigned)create_mode, (unsigned)mask,
           (unsigned)(st.st_mode & 0777),
           (unsigned)(create_mode & ~mask & 0777));
    unlink(path);
}

int main(void)
{
    create_show("/tmp/tlpi_umask_a", 0666, 0022);
    create_show("/tmp/tlpi_umask_b", 0666, 0077);
    create_show("/tmp/tlpi_umask_c", 0777, 0022);
    return 0;
}
