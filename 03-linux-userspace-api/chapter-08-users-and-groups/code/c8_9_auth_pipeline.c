/* c8_9_auth_pipeline.c —— 8.5：把整章串成一条可自动跑的认证流水线
 *
 * 原书 Listing 8-2 需要交互输入，无法在评测环境里跑；本程序是它的
 * 可自动化等价物：自己造一份合成 shadow，再走 getspnam()/fgetspent()
 * 把记录读回来，逐条验证候选口令。
 *
 * 编译：cc -Wall -Wextra -o c8_9_auth_pipeline c8_9_auth_pipeline.c -lcrypt
 */
#define _DEFAULT_SOURCE
#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 安全版校验：副本作盐 + 排除锁定账号 + 排除 *0 与 *1 哨兵（同 c8_8_crypt_traps.c） */
static int verify(const char *password, const char *stored)
{
    char salt[256];

    if (password == NULL || stored == NULL)
        return 0;
    if (snprintf(salt, sizeof salt, "%s", stored) >= (int) sizeof salt)
        return 0;
    if (salt[0] == '!' || salt[0] == '*')
        return 0;

    char *got = crypt(password, salt);
    if (got == NULL || got[0] == '*')
        return 0;

    return strcmp(got, salt) == 0;
}

static int days_since_epoch(void)
{
    return (int) (time(NULL) / 86400);
}

int main(void)
{
    /* ---- 1. 造一份合成 shadow（alice 的密文现算） ---- */
    char alice_hash[256];
    snprintf(alice_hash, sizeof alice_hash, "%s", crypt("hunter2", "$6$tlpidemo$"));

    const char *sf = "/tmp/c8_shadow_pipeline";
    FILE *w = fopen(sf, "w");
    if (w == NULL) {
        perror("fopen for write");
        return 1;
    }
    fprintf(w, "alice:%s:%d:0:99999:7:14:21000\n", alice_hash, days_since_epoch());
    fputs("bob:!:19500:0:99999:7:::\n", w);         /* 锁定 */
    fputs("carol:*:19500:0:99999:7:::\n", w);       /* 无口令 */
    fclose(w);

    printf("今天距 1970-01-01 = %d 天\n", days_since_epoch());
    printf("alice 的密文（$6$ 前缀 = SHA-512-crypt）= %s\n\n", alice_hash);

    /* ---- 2. 走 shadow 查询路径读回来，逐条验证 ---- */
    struct { const char *user, *pw; } attempts[] = {
        { "alice", "hunter2" },     /* 正确 */
        { "alice", "Hunter2" },     /* 大小写不同 */
        { "alice", "" },            /* 空口令 */
        { "bob",   "hunter2" },     /* 账号被 ! 锁定 */
        { "carol", "hunter2" },     /* 账号被 * 禁用 */
    };

    FILE *f = fopen(sf, "r");
    struct spwd *sp;
    while (f != NULL && (sp = fgetspent(f)) != NULL) {
        int cnt = sp->sp_max;
        const char *policy = (cnt == 0 || cnt >= 99999)
                                 ? "永不过期（0 或 99999 都表示不检查）"
                                 : "有最长有效期";
        printf("账号 %-6s 密文前缀=%-4.4s 最长有效=%ld 天（%s）\n",
               sp->sp_namp, sp->sp_pwdp, sp->sp_max, policy);

        for (size_t i = 0; i < sizeof attempts / sizeof attempts[0]; i++) {
            if (strcmp(attempts[i].user, sp->sp_namp) != 0)
                continue;
            printf("   口令 \"%s\" -> %s\n", attempts[i].pw,
                   verify(attempts[i].pw, sp->sp_pwdp) ? "✅ 认证通过" : "❌ 拒绝");
        }
    }
    if (f != NULL)
        fclose(f);

    /* ---- 3. 真实环境对照：本容器以 root 运行，能读到真的 shadow ---- */
    puts("\n-- 真实 shadow（本容器） --");
    struct passwd *pw = getpwnam("ce");
    if (pw == NULL) {
        puts("getpwnam(\"ce\") 失败，跳过");
        return 0;
    }
    struct spwd *real = getspnam(pw->pw_name);
    if (real == NULL) {
        printf("getspnam(\"%s\") = NULL —— 普通用户读 shadow 会 errno=EACCES\n",
               pw->pw_name);
    } else {
        printf("%s 的密文 = %s\n", real->sp_namp, real->sp_pwdp);
        printf("前缀 $1$ = MD5-crypt。它的验证速度比 $6$ 快一个数量级（见 c8_7_crypt_cost.c\n");
        printf("里的同机实测表），所以这种哈希在字典攻击面前最脆弱 ——\n");
        printf("这正是现代发行版把默认算法换成 $6$/$y$ 的原因。\n");
        printf("账号未锁定：%s\n", verify("wrong-password", real->sp_pwdp) ? "错误口令也通过了（！）"
                                                                          : "错误口令被正确拒绝");
    }
    return 0;
}
