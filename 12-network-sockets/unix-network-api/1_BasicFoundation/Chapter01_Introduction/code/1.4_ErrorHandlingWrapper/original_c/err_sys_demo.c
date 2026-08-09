/*
 * UNP 1.4 · err_sys 典型逻辑（教学简化版）
 *
 * 原书 libunp 中 err_sys 还会输出文件名/行号等；此处只保留核心：
 *   1. 立刻保存 errno
 *   2. 打印 msg + strerror
 *   3. exit(1)
 *
 * 编译本文件仅用于阅读逻辑，不与 libunp 混用时可：
 *   gcc -c err_sys_demo.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void
err_sys(const char *msg)
{
    int err = errno;   /* 必须在任何可能改 errno 的调用之前保存 */

    fprintf(stderr, "%s: %s\n", msg, strerror(err));
    exit(1);
}

/*
 * err_quit：业务错误，不依赖 errno（1.2 里 argc!=2 用 err_quit）
 * void err_quit(const char *fmt, ...) { vfprintf(...); exit(1); }
 *
 * Pthread 失败示例（错误在返回值 n，不是 errno）：
 *   if ((n = pthread_create(...)) != 0)
 *       err_exit(n, "pthread_create error");
 */
