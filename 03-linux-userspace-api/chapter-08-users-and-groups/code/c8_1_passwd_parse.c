/* c8_1_passwd_parse.c —— 8.1：/etc/passwd 的 7 字段格式
 *
 * 手工按 ':' 切分每一行，再和 getpwnam() 的结果逐字段对照：
 * 「账户库 API」说到底就是对这几个文本文件的封装。
 *
 * 编译：cc -Wall -Wextra -o c8_1_passwd_parse c8_1_passwd_parse.c
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NFIELD 7

static const char *fieldname[NFIELD] = {
    "登录名", "密文占位", "UID", "主 GID", "注释(GECOS)", "家目录", "登录 shell"
};

/* 就地把 line 按 ':' 切成最多 NFIELD 段，把 ':' 改成 '\0'；返回实际段数 */
static int split_fields(char *line, char **f)
{
    int n = 0;
    char *p = line;

    if (n < NFIELD)
        f[n++] = p;
    for (; *p != '\0' && n < NFIELD; p++) {
        if (*p == ':') {
            *p = '\0';
            f[n++] = p + 1;
        }
    }
    return n;
}

int main(void)
{
    FILE *fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        perror("fopen /etc/passwd");
        return 1;
    }

    char line[512];
    int rec = 0;
    while (fgets(line, sizeof line, fp) != NULL) {
        size_t len = strlen(line);
        int had_nl = (len > 0 && line[len - 1] == '\n');

        if (had_nl)
            line[len - 1] = '\0';       /* 自己剥掉换行，别让 strtol 撞上 */

        rec++;
        char *f[NFIELD];
        int n = split_fields(line, f);
        printf("== 记录 %d：字段数 %d%s\n", rec, n,
               had_nl ? "" : "（文件末行没有换行符）");
        for (int i = 0; i < n; i++)
            printf("   %d %-12s = %s\n", i + 1, fieldname[i], f[i]);

        if (n >= 3) {
            char *end;
            long uid = strtol(f[2], &end, 10);
            printf("   -> strtol(f[2]) = %ld（UID 在文件里只是十进制文本）\n", uid);
        }
    }
    fclose(fp);
    printf("共读到 %d 条记录\n", rec);

    /* ---- 同一份数据，走 API ---- */
    struct passwd *pw = getpwnam("ce");
    if (pw != NULL) {
        printf("\ngetpwnam(\"ce\") -> pw_name=%s pw_passwd=%s pw_uid=%ld pw_gid=%ld\n",
               pw->pw_name, pw->pw_passwd, (long) pw->pw_uid, (long) pw->pw_gid);
        printf("                    pw_gecos=%s pw_dir=%s pw_shell=%s\n",
               pw->pw_gecos, pw->pw_dir, pw->pw_shell);
        printf("pw_passwd 是占位符还是真密文：%s\n",
               strcmp(pw->pw_passwd, "x") == 0 ? "占位符 \"x\"" : "真密文");
    } else {
        printf("\ngetpwnam(\"ce\") -> NULL\n");
    }

    /* ---- 「UID 0」不等于「passwd 里有 0 号记录」 ---- */
    errno = 0;
    pw = getpwuid(0);
    printf("getpwuid(0) -> %s（errno=%d）\n", pw ? pw->pw_name : "NULL", errno);

    return 0;
}
