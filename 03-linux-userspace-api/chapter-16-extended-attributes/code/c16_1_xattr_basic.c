/* c16_1_xattr_basic.c — Ch16 §16.3：setxattr/getxattr/listxattr/removexattr 四件套
 *
 * TLPI §16.3 必须实测钉住的点：
 *   ① xattr 是 name=value 对：名字带 namespace 前缀（user.），值是**二进制安全**的
 *      字节串——不解释、不补 NUL，size 自己管；
 *   ② getxattr 用 buf=NULL 探大小：返回值是"所需字节数"；缓冲小了 ERANGE，
 *      属性不存在 ENOATTR——三种返回都要分清；
 *   ③ listxattr 返回的是 **NUL 分隔的名字串拼接**（不是指针数组），
 *      返回值同样是"所需字节数"；
 *   ④ value 是二进制：存 struct、存 int 都合法（demo 里存一个 double）；
 *   ⑤ 同名重设 = 覆盖（默认语义），removexattr 之后 getxattr → ENOATTR。
 *
 * 本 demo 用移植宏抹平 Linux/macOS 的签名差异（macOS 多 position/options 参数）。
 * 编译： cc -Wall -Wextra -o c16_1_xattr_basic c16_1_xattr_basic.c
 * 取材： man-pages 6.19 xattr(7)（namespace 规则）+ getxattr(3)（ENOATTR/ERANGE）
 *       TLPI §16.1/§16.3
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/xattr.h>
#define XSET(p,n,v,s)   setxattr(p,n,v,s,0,0)
#define XGET(p,n,v,s)   getxattr(p,n,v,s,0,0)
#define XLIST(p,l,s)    listxattr(p,l,s,0)
#define XRM(p,n)        removexattr(p,n,0)
#define ENOATTR_NAME    "user.comment"
#else
#include <sys/xattr.h>
#define XSET(p,n,v,s)   setxattr(p,n,v,s,0)
#define XGET(p,n,v,s)   getxattr(p,n,v,s)
#define XLIST(p,l,s)    listxattr(p,l,s)
#define XRM(p,n)        removexattr(p,n)
#define ENOATTR_NAME    ENODATA       /* Linux 上属性不存在返回 ENODATA */
#endif

int main(void)
{
    const char *path = "/tmp/tlpi_c16_1.txt";
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("fopen"); return EXIT_FAILURE; }
    fputs("xattr demo file\n", fp);
    fclose(fp);

    /* ---------- ①② set + 探大小 + 取值 ---------- */
    printf("== ①② set/get：值是二进制安全的字节串 ==\n");
    const char *comment = "created by c16_1, TLPI ch16 demo";
    if (XSET(path, "user.comment", comment, strlen(comment)) == -1) {
        perror("setxattr user.comment"); return EXIT_FAILURE;
    }
    /* 存一个 double：证明 value 不解释内容 */
    double price = 12345.6789;
    if (XSET(path, "user.price", &price, sizeof price) == -1) {
        perror("setxattr user.price"); return EXIT_FAILURE;
    }

    ssize_t need = XGET(path, "user.comment", NULL, 0);   /* 探大小 */
    printf("  getxattr(buf=NULL) → %zd（属性值所需字节数）\n", need);
    char buf[128];
    ssize_t got = XGET(path, "user.comment", buf, sizeof buf);
    printf("  getxattr 实取 %zd 字节: \"%.*s\"\n", got, (int) got, buf);

    double price_back = 0;
    XGET(path, "user.price", &price_back, sizeof price_back);
    printf("  user.price 读回 double = %.4f（二进制往返无损）\n\n", price_back);

    /* ---------- 缓冲太小的 ERANGE ---------- */
    printf("== 缓冲太小：ERANGE ==\n");
    char tiny[4];
    errno = 0;
    ssize_t r = XGET(path, "user.comment", tiny, sizeof tiny);
    printf("  getxattr(4 字节缓冲) = %zd errno=%d(%s)\n\n",
           r, errno, strerror(errno));

    /* ---------- ③ listxattr：NUL 拼接串 ---------- */
    printf("== ③ listxattr：名字串 NUL 拼接，不是指针数组 ==\n");
    char list[1024];
    ssize_t lneed = XLIST(path, NULL, 0);
    ssize_t llen = XLIST(path, list, sizeof list);
    printf("  探大小=%zd，实取=%zd，逐个打印：\n", lneed, llen);
    for (ssize_t i = 0; i < llen; ) {
        const char *name = list + i;
        size_t len = strlen(name);
        if (strncmp(name, "user.", 5) == 0)
            printf("    [%s]\n", name);
        i += len + 1;
    }
    printf("  ⚠️ 名字串里 user.* 之外还可能有系统自己放的属性（如 macOS 的\n");
    printf("     com.apple.*），逐个按前缀过滤；返回值是「还需要多少字节」。\n\n");

    /* ---------- ⑤ 同名覆盖与删除 ---------- */
    printf("== ⑤ 同名重设=覆盖；removexattr 后 ENOATTR ==\n");
    const char *v2 = "shorter";
    XSET(path, "user.comment", v2, strlen(v2));
    got = XGET(path, "user.comment", buf, sizeof buf);
    printf("  重设后 user.comment = \"%.*s\"（%zd 字节，旧值被顶掉）\n",
           (int) got, buf, got);
    if (XRM(path, "user.comment") == -1) { perror("removexattr"); return EXIT_FAILURE; }
    errno = 0;
    r = XGET(path, "user.comment", buf, sizeof buf);
    printf("  删除后 getxattr = %zd errno=%d(%s)\n",
           r, errno, strerror(errno));
    printf("  ⚠️ 「属性不存在」Linux 返回 ENODATA(61)，macOS 返回 ENOATTR(93)——\n");
    printf("     名字与**数值都不同**，可移植代码必须两个宏都判（或判 errno==\n");
    printf("     ENODATA || errno==ENOATTR）。本机实测 errno=93 即 ENOATTR。\n");

    unlink(path);
    return EXIT_SUCCESS;
}
