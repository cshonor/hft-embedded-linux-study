/* c8_8_crypt_traps.c —— 8.5：crypt() 的三个坑，其中一个是安全漏洞
 *
 * 坑 1：crypt() 和 getpwnam() 一样返回静态缓冲 —— 前一次的结果会被后一次改写
 * 坑 2：把 crypt() 自己返回的指针当「盐」再喂回去，crypt() 会失败返回 "*1"
 * 坑 3：此时 strcmp(返回值, stored) 两边指向同一块内存 → 恒等 → 任何口令都"通过"
 *
 * 编译：cc -Wall -Wextra -o c8_8_crypt_traps c8_8_crypt_traps.c -lcrypt
 */
#define _DEFAULT_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <string.h>

/* ❌ 看起来和原书 Listing 8-2 一模一样，但存的是 crypt() 自己的返回值时就废了 */
static int bad_verify(const char *password, const char *stored)
{
    return strcmp(crypt(password, stored), stored) == 0;
}

/* ✅ 正确写法：盐拷成副本、先挡锁定账号、再挡 *0 与 *1 这两个失败哨兵 */
static int good_verify(const char *password, const char *stored)
{
    char salt[256];

    if (password == NULL || stored == NULL)
        return 0;
    if (snprintf(salt, sizeof salt, "%s", stored) >= (int) sizeof salt)
        return 0;                       /* 密文长得离谱，直接拒绝 */
    if (salt[0] == '!' || salt[0] == '*')
        return 0;                       /* 锁定 / 无口令 */

    char *got = crypt(password, salt);
    if (got == NULL)
        return 0;
    if (got[0] == '*')
        return 0;                       /* crypt 失败（*0 / *1），绝不当成匹配 */

    return strcmp(got, salt) == 0;
}

int main(void)
{
    /* ---- 坑 1：静态缓冲 ---- */
    char *h1 = crypt("pw-one", "$6$saltAAAA");
    char keep1[256];
    snprintf(keep1, sizeof keep1, "%s", h1);    /* 先拷一份 */

    char *h2 = crypt("pw-two", "$6$saltBBBB");
    printf("h1=%p  h2=%p  (%s)\n", (void *) h1, (void *) h2,
           h1 == h2 ? "同一块内存" : "不同内存");
    printf("h1 现在指向的内容 = %.22s...\n", h1);
    printf("调用前拷下来的 keep1 = %.22s...\n", keep1);

    /* ---- 坑 2 + 坑 3：别名导致「任何口令都通过」 ---- */
    char *stored_ptr = crypt("secret123", "$6$rounds=5000$abc");
    char stored_copy[256];
    snprintf(stored_copy, sizeof stored_copy, "%s", stored_ptr);

    printf("\nstored_ptr（crypt 静态缓冲）= %.30s...\n", stored_ptr);
    printf("stored_copy（副本）          = %.30s...\n", stored_copy);

    printf("\n[❌ bad_verify] 直接把 crypt 的返回值当 stored\n");
    printf("bad_verify(\"definitely-wrong\", stored_ptr) = %d   <- 错口令本该返回 0\n",
           bad_verify("definitely-wrong", stored_ptr));
    printf("此刻 stored_ptr 的内容变成了 = %s\n", stored_ptr);

    printf("\n[✅ good_verify] 用副本，且检查哨兵\n");
    printf("good_verify(\"secret123\", stored_copy)        = %d\n",
           good_verify("secret123", stored_copy));
    printf("good_verify(\"definitely-wrong\", stored_copy) = %d\n",
           good_verify("definitely-wrong", stored_copy));
    printf("good_verify(\"x\", \"!\")   （锁定账号）          = %d\n",
           good_verify("x", "!"));
    printf("good_verify(\"x\", \"*\")   （无口令）            = %d\n",
           good_verify("x", "*"));

    puts("\n一句话：认证代码里绝不能把 crypt() 的返回值再当盐喂回去；");
    puts("        也不能只看 strcmp 的结果，必须先排除 *0/*1 与 !/* 这两类哨兵。");
    return 0;
}
