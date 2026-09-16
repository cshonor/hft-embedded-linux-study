/* ex16_1_setfattr.c — 习题 16-1：简易版 setfattr(1)
 *
 * 题面：写一个程序创建/修改文件的 **user** 扩展属性（setfattr(1) 的
 * 简化版），文件名、属性名、属性值都从命令行给出。
 *
 * 用法：./ex16_1_setfattr file name value
 *   （属性名不带 user. 前缀时自动补上；带前缀则要求它就是 user.*）
 *
 * 编译： cc -Wall -Wextra -o ex16_1_setfattr ex16_1_setfattr.c
 * 取材： TLPI §16.3 习题 16-1；setfattr(1) 的语义
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(__APPLE__)
#include <sys/xattr.h>
#define XSET(p,n,v,s)   setxattr(p,n,v,s,0,0)
#define XGET(p,n,v,s)   getxattr(p,n,v,s,0,0)
#else
#include <sys/xattr.h>
#define XSET(p,n,v,s)   setxattr(p,n,v,s,0)
#define XGET(p,n,v,s)   getxattr(p,n,v,s)
#endif

int main(int argc, char *argv[])
{
    if (argc != 4 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s file name value\n"
                "       name 会被限定进 user.* 命名空间\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *file = argv[1], *name = argv[2], *value = argv[3];
    char fullname[256];

    if (strncmp(name, "user.", 5) == 0) {
        snprintf(fullname, sizeof fullname, "%s", name);
    } else {
        snprintf(fullname, sizeof fullname, "user.%s", name);
    }

    if (XSET(file, fullname, value, strlen(value)) == -1) {
        fprintf(stderr, "setxattr %s: %s\n", fullname, strerror(errno));
        return EXIT_FAILURE;
    }

    /* 回读验证 */
    char buf[512];
    ssize_t got = XGET(file, fullname, buf, sizeof buf);
    if (got < 0) {
        fprintf(stderr, "getxattr %s: %s\n", fullname, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("%s: %s = \"%.*s\" (%zd bytes) 已写入并回读验证\n",
           file, fullname, (int) got, buf, got);
    return EXIT_SUCCESS;
}
