/* c8_5_lookup_r.c —— 8.4：_r 版本（可重入）与 ERANGE 重试
 *
 * 非 _r 版本把结果放在静态缓冲里；_r 版本由调用者提供 struct 与缓冲，
 * 因此线程安全。代价是要自己处理「缓冲不够」这一种全新的错误。
 *
 * 编译：cc -Wall -Wextra -o c8_5_lookup_r c8_5_lookup_r.c
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    long pw_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    long gr_max = sysconf(_SC_GETGR_R_SIZE_MAX);
    printf("_SC_GETPW_R_SIZE_MAX=%ld  _SC_GETGR_R_SIZE_MAX=%ld\n", pw_max, gr_max);

    struct passwd pw, *res = NULL;
    char *buf = malloc((size_t) pw_max);

    /* ---- 成功路径 ---- */
    int s = getpwnam_r("ce", &pw, buf, (size_t) pw_max, &res);
    printf("getpwnam_r(\"ce\", bufsize=%ld) -> s=%d res=%s", pw_max, s,
           res ? "non-NULL" : "NULL");
    if (res != NULL)
        printf("  pw_name=%s pw_uid=%ld pw_gecos=%s", pw.pw_name,
               (long) pw.pw_uid, pw.pw_gecos);
    else if (s == ENOENT || s == ESRCH)
        printf("  （errno 风格：s=ENOENT 表示不存在）");
    putchar('\n');

    /* ---- 「不存在」在 _r 里的表达方式变了 ---- */
    res = (struct passwd *) 0x1;        /* 故意塞个非 NULL，看库里会不会改它 */
    s = getpwnam_r("nosuchuser", &pw, buf, (size_t) pw_max, &res);
    printf("getpwnam_r(\"nosuchuser\")     -> s=%d res=%s"
           "   （s==0 且 res==NULL 才是「不存在」；只看 s 是不够的）\n",
           s, res ? "non-NULL" : "NULL");
    free(buf);

    /* ---- 缓冲太小：ERANGE，指数增长重试 ---- */
    puts("\n-- 缓冲从小往大试，直到不再 ERANGE --");
    size_t sz = 8;
    while (sz <= 8192) {
        char *b = malloc(sz);
        if (b == NULL)
            break;
        res = NULL;
        s = getpwnam_r("ce", &pw, b, sz, &res);
        printf("bufsize=%-5zu -> s=%-3d %-28s res=%s\n", sz, s,
               s == ERANGE ? "ERANGE（缓冲不够）" : strerror(s),
               res ? "non-NULL" : "NULL");
        free(b);
        if (s != ERANGE)
            break;
        sz *= 2;
    }

    puts("\n注意：_r 版本失败时返回的是「错误号」，不是 -1；");
    puts("      它不会去动 errno —— 所以不要再对它用 errno 范式。");
    return 0;
}
