/* check_password.c —— 原书 Listing 8-2（p.164）复刻版
 *
 * 读一个用户名 + 口令，与 /etc/shadow 里的密文比对。
 * 编译：cc -Wall -Wextra -o check_password check_password.c -lcrypt
 *
 * 与原书的差别（三处，主流程逐行保持原样）：
 *   1. 原书 #include "tlpi_hdr.h" 取 errExit()/fatal()/Boolean —— 本文件内联了
 *      最小等价实现（见文末），并显式补上原书由 tlpi_hdr.h 间接引入的
 *      <errno.h>/<stdio.h>/<stdlib.h>/<string.h>。
 *   2. 原书的宏块用 _BSD_SOURCE 取 getpass(3) 声明、_XOPEN_SOURCE 取 crypt()
 *      声明。glibc 2.20 起 _BSD_SOURCE 已废弃；按 man 3 crypt 的 Feature Test
 *      Macro 表，crypt() 自 glibc 2.28 起只需 _DEFAULT_SOURCE。
 *   3. Boolean authOk 改成 int authOk（省掉书中定义的 Boolean）。
 *
 * 注意：本程序必须交互输入（stdin 读到 EOF 即退出）。可自动化的等价物
 * 见 c8_9_auth_pipeline.c。
 */
/* Compile with -lcrypt */
#if ! defined(__sun)
#define _DEFAULT_SOURCE /* 原书用 _BSD_SOURCE + _XOPEN_SOURCE（均已过时） */
#endif
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void errExit(const char *msg);
static void fatal(const char *msg);

int
main(int argc, char *argv[])
{
    char *username, *password, *encrypted, *p;
    struct passwd *pwd;
    struct spwd *spwd;
    int authOk;
    size_t len;
    long lnmax;

    /* Determine size of buffer required for a username, and allocate it */

    lnmax = sysconf(_SC_LOGIN_NAME_MAX);
    if (lnmax == -1)                    /* If limit is indeterminate */
        lnmax = 256;                    /* make a guess */

    username = malloc(lnmax);
    if (username == NULL)
        errExit("malloc");

    printf("Username: ");
    fflush(stdout);
    if (fgets(username, lnmax, stdin) == NULL)
        exit(EXIT_FAILURE);             /* Exit on EOF */

    len = strlen(username);
    if (username[len - 1] == '\n')
        username[len - 1] = '\0';       /* Remove trailing '\n' */

    /* Look up password and shadow password records for username */

    pwd = getpwnam(username);
    if (pwd == NULL)
        fatal("couldn't get password record");
    spwd = getspnam(username);
    if (spwd == NULL && errno == EACCES)
        fatal("no permission to read shadow password file");

    if (spwd != NULL)           /* If there is a shadow password record */
        pwd->pw_passwd = spwd->sp_pwdp;     /* Use the shadow password */

    password = getpass("Password: ");

    /* Encrypt password and erase cleartext version immediately */

    encrypted = crypt(password, pwd->pw_passwd);
    for (p = password; *p != '\0'; )
        *p++ = '\0';

    if (encrypted == NULL)
        errExit("crypt");

    authOk = strcmp(encrypted, pwd->pw_passwd) == 0;
    if (!authOk) {
        printf("Incorrect password\n");
        exit(EXIT_FAILURE);
    }

    printf("Successfully authenticated: UID=%ld\n", (long) pwd->pw_uid);

    /* Now do authenticated work... */

    exit(EXIT_SUCCESS);
}

/* ---- 以下是原书 tlpi_hdr.h 提供、本文件内联的最小等价实现 ---- */

static void
errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void
fatal(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}
