/* c8_6_crypt_schemes.c —— 8.5：crypt() 的算法由盐决定
 *
 * 同一个口令配不同的盐，输出长度、前缀、算法全都不一样。
 * 盐不是「随机数」，它是「算法标识 + 轮数参数 + 随机前缀」的打包串。
 *
 * 编译：cc -Wall -Wextra -o c8_6_crypt_schemes c8_6_crypt_schemes.c -lcrypt
 */
#define _DEFAULT_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <string.h>

/* 从密文前缀反推算法 —— 认证程序要干的第一件事 */
static const char *algo_of(const char *h)
{
    if (h == NULL)
        return "NULL（crypt 失败）";
    if (h[0] == '*')
        return "失败哨兵 *0 / *1";
    if (h[0] != '$')
        return "DES-crypt（13 字符、无前缀）";
    if (strncmp(h, "$1$", 3) == 0)
        return "MD5-crypt";
    if (strncmp(h, "$5$", 3) == 0)
        return "SHA-256-crypt";
    if (strncmp(h, "$6$", 3) == 0)
        return "SHA-512-crypt";
    if (strncmp(h, "$2", 2) == 0)
        return "bcrypt";
    if (strncmp(h, "$y$", 3) == 0)
        return "yescrypt";
    return "未知";
}

int main(void)
{
    const char *salt[] = {
        "ab",                            /* 经典 DES：2 字符盐 */
        "$1$abc",                        /* MD5 */
        "$5$abc",                        /* SHA-256，默认 5000 轮 */
        "$6$abc",                        /* SHA-512，默认 5000 轮 */
        "$6$rounds=1000$abc",            /* 显式指定轮数 */
        "$2b$08$abcdefghijklmnopqrstuv", /* bcrypt cost=8 */
        "$y$j9T$abcdefghij",             /* yescrypt（新发行版默认） */
    };

    for (size_t i = 0; i < sizeof salt / sizeof salt[0]; i++) {
        char *h = crypt("secret123", salt[i]);
        printf("\n盐 %-32s\n  密文 %s\n  长度 %zu  算法 %s\n",
               salt[i], h ? h : "NULL", h ? strlen(h) : 0, algo_of(h));
    }

    /* ---- 盐相同 → 结果必然相同（这就是不能只靠加密防彩虹表的原因） ---- */
    char d1[256], d2[256];
    snprintf(d1, sizeof d1, "%s", crypt("secret123", "$6$fixedsalt"));
    snprintf(d2, sizeof d2, "%s", crypt("secret123", "$6$fixedsalt"));
    printf("\n同一口令 + 同一盐两次：%s\n", strcmp(d1, d2) == 0 ? "完全相同" : "不同");

    snprintf(d1, sizeof d1, "%s", crypt("secret123", "$6$fixedsalt"));
    snprintf(d2, sizeof d2, "%s", crypt("secret123", "$6$otherslat"));
    printf("同一口令 + 不同盐：  %s\n", strcmp(d1, d2) == 0 ? "相同" : "不同");

    /* ---- DES 只吃口令的前 8 个字符 ---- */
    char e1[64], e2[64], e3[64];
    snprintf(e1, sizeof e1, "%s", crypt("12345678AAA", "ab"));
    snprintf(e2, sizeof e2, "%s", crypt("12345678ZZZ", "ab"));
    snprintf(e3, sizeof e3, "%s", crypt("1234567", "ab"));
    printf("\nDES \"12345678AAA\" -> %s\n", e1);
    printf("DES \"12345678ZZZ\" -> %s  %s\n", e2,
           strcmp(e1, e2) == 0 ? "（一模一样：第 9 个字符根本没进算法）" : "（不同）");
    printf("DES \"1234567\"     -> %s  %s\n", e3,
           strcmp(e1, e3) == 0 ? "（也一模一样：不足 8 字符按 0 补齐）" : "（不同）");

    /* ---- 锁定账号：密文以 ! 或 * 开头 ---- */
    puts("\n-- 锁定 / 禁用账号的哨兵 --");
    const char *lock[] = { "!", "!!", "*", "!$6$abc" };
    for (size_t i = 0; i < sizeof lock / sizeof lock[0]; i++) {
        char *h = crypt("anything", lock[i]);
        printf("stored=\"%-8s\" -> crypt() 返回 \"%s\"  %s\n", lock[i],
               h ? h : "NULL", algo_of(h));
    }
    puts("（*0 永远不可能等于 stored 的密文，所以认证必然失败 —— 这就是锁定的实现方式）");
    return 0;
}
