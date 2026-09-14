/* t_getpwnam_r.c —— 原书 Ch08 配套程序复刻版
 *
 * 原书说明：This file is not printed in the book; it is a supplementary file
 * for Chapter 8.（即它不是 Listing，只是随章发布的示例。）
 *
 * 演示 getpwnam_r()：可重入版本，结果写进调用者提供的 struct passwd 与缓冲。
 * 原书依赖 tlpi_hdr.h（取 usageErr/errExitEN/malloc 包装），此处内联最小实现，
 * 主流程逐行保持原样。
 *
 * 注意原书这一行打印的是 pw_gecos（注释字段），不是 pw_name：
 *     printf("Name: %s\n", pwd.pw_gecos);
 * 已核对 man7.org 上的官方源码，确实如此；man 3 getpwnam 的 EXAMPLES 里
 * 也是 "Name: %s" 配 pwd.pw_gecos。对本容器的 ce 账号，pw_gecos =
 * "Not a real account" —— 输出看着像出错，其实标签与字段本来就不对应。
 * 详见 notes/8.4-retrieving-info.md。
 *
 * 编译：cc -Wall -Wextra -o t_getpwnam_r t_getpwnam_r.c
 */
#define _DEFAULT_SOURCE
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s username\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    size_t bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    char *buf = malloc(bufSize);
    if (buf == NULL) {
        fprintf(stderr, "malloc %zu failed\n", bufSize);
        exit(EXIT_FAILURE);
    }

    struct passwd *result;
    struct passwd pwd;

    int s = getpwnam_r(argv[1], &pwd, buf, bufSize, &result);
    if (s != 0) {
        fprintf(stderr, "getpwnam_r: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    if (result != NULL)
        printf("Name: %s\n", pwd.pw_gecos);
    else
        printf("Not found\n");

    exit(EXIT_SUCCESS);
}
