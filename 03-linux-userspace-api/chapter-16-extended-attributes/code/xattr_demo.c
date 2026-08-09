/* user.* xattr CRUD + listxattr walk.
 * cc -Wall -Wextra -o xattr_demo xattr_demo.c
 * ./xattr_demo [/tmp/tlpi_xattr_demo.txt]
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h>
#include <unistd.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void list_all(const char *path)
{
    ssize_t len, i;
    char *buf;

    len = listxattr(path, NULL, 0);
    if (len == -1)
        die("listxattr(size)");
    if (len == 0) {
        printf("  (no xattrs)\n");
        return;
    }

    buf = malloc((size_t)len);
    if (buf == NULL)
        die("malloc");
    len = listxattr(path, buf, (size_t)len);
    if (len == -1) {
        free(buf);
        die("listxattr");
    }

    for (i = 0; i < len; ) {
        const char *key = buf + i;
        char val[256];
        ssize_t vlen;

        vlen = getxattr(path, key, val, sizeof(val) - 1);
        if (vlen == -1) {
            printf("  %s = <get failed: %s>\n", key, strerror(errno));
        } else {
            val[vlen] = '\0';
            printf("  %s = \"%s\" (%zd bytes)\n", key, val, vlen);
        }
        i += (ssize_t)strlen(key) + 1;
    }
    free(buf);
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/tmp/tlpi_xattr_demo.txt";
    const char *k1 = "user.comment";
    const char *k2 = "user.uploader";
    int fd;
    char val[128];
    ssize_t n;

    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1)
        die("open");
    if (write(fd, "xattr demo\n", 11) != 11)
        die("write");
    close(fd);

    printf("file: %s\n", path);

    if (setxattr(path, k1, "hello", 5, 0) == -1)
        die("setxattr comment");
    if (setxattr(path, k2, "alice", 5, XATTR_CREATE) == -1)
        die("setxattr uploader CREATE");

    /* REPLACE should succeed */
    if (setxattr(path, k1, "hello-v2", 8, XATTR_REPLACE) == -1)
        die("setxattr REPLACE");

    /* CREATE on existing key -> EEXIST */
    if (setxattr(path, k1, "x", 1, XATTR_CREATE) == -1)
        printf("CREATE on existing: %s (expected EEXIST)\n", strerror(errno));

    n = getxattr(path, k1, NULL, 0);
    if (n == -1)
        die("getxattr size");
    printf("getxattr(%s) size probe = %zd\n", k1, n);

    n = getxattr(path, k1, val, sizeof(val) - 1);
    if (n == -1)
        die("getxattr");
    val[n] = '\0';
    printf("getxattr(%s) = \"%s\"\n", k1, val);

    printf("list:\n");
    list_all(path);

    if (removexattr(path, k2) == -1)
        die("removexattr");
    printf("after remove %s:\n", k2);
    list_all(path);

    /* trusted.* should fail for non-root */
    if (setxattr(path, "trusted.secret", "x", 1, 0) == -1)
        printf("set trusted.* as non-root: %s (expected EPERM)\n",
               strerror(errno));

    return 0;
}
