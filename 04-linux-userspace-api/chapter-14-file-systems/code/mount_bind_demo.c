/* MS_BIND demo — requires CAP_SYS_ADMIN / root.
 * Creates /tmp/tlpi-bind-{src,mnt}, bind-mounts, then umounts.
 *
 * cc -Wall -Wextra -o mount_bind_demo mount_bind_demo.c
 * sudo ./mount_bind_demo
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    const char *src = "/tmp/tlpi-bind-src";
    const char *mnt = "/tmp/tlpi-bind-mnt";
    char path[256];
    FILE *fp;

    if (mkdir(src, 0755) == -1 && errno != EEXIST)
        die("mkdir src");
    if (mkdir(mnt, 0755) == -1 && errno != EEXIST)
        die("mkdir mnt");

    snprintf(path, sizeof(path), "%s/hello.txt", src);
    fp = fopen(path, "w");
    if (fp == NULL)
        die("fopen");
    fputs("bind-mount ok\n", fp);
    fclose(fp);

    /* bind: source is a directory; fstype/data unused */
    if (mount(src, mnt, NULL, MS_BIND, NULL) == -1)
        die("mount(MS_BIND) — need root?");

    printf("bind-mounted %s -> %s\n", src, mnt);
    snprintf(path, sizeof(path), "%s/hello.txt", mnt);
    fp = fopen(path, "r");
    if (fp == NULL)
        die("fopen via mnt");
    {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp))
            printf("read via mountpoint: %s", buf);
    }
    fclose(fp);

    if (umount(mnt) == -1)
        die("umount");
    printf("umounted %s\n", mnt);
    return 0;
}
