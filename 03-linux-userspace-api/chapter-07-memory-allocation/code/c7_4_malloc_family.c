/* Ch7 §7.1 — malloc / calloc / realloc / malloc(0) 的语义实测
 * 编译: gcc -O2 -Wall -Wextra -o c7_4_malloc_family c7_4_malloc_family.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(void)
{
    /* 1) malloc(0)：glibc 返回可 free 的非空指针，不能当失败处理 */
    void *z = malloc(0);
    printf("1) malloc(0) = %p (%s)\n", z,
           z ? "非 NULL，必须 free" : "NULL");
    free(z);

    /* 2) malloc 不清零 —— 但 glibc 的 tcache 会覆写块的头部 16 字节 */
    char *a = malloc(32);
    memcpy(a, "SECRET-TOKEN-1234", 17);
    memcpy(a + 16, "LEFTOVER!", 9);     /* 放到 tcache 元数据之后 */
    printf("2) free 后重新 malloc：a = %p\n", (void *) a);
    free(a);
    char *b = malloc(32);               /* 大概率拿到同一个块 */
    printf("   b = %p  (与 a 同址 = %s)\n", (void *) b, a == b ? "是" : "否");
    printf("   b 的前 32 字节:");
    for (int i = 0; i < 32; i++)
        printf(" %02x", (unsigned char) b[i]);
    printf("\n");
    printf("   b[16] 起的字符串 = %.8s   <- 残留未被清零\n", b + 16);
    free(b);

    /* 3a) calloc 清零 */
    int *arr = calloc(8, sizeof(int));
    printf("3a) calloc(8, 4) 全零 = %s\n",
           (arr[0] | arr[7]) == 0 ? "是" : "否");
    free(arr);

    /* 3b) calloc 的溢出保护：n*s 回绕时返回 NULL，而不是「成功」一个小块。
     * 用非 const 变量传参，绕开编译器的静态检查，让行为在运行期体现。 */
    volatile size_t nmemb = SIZE_MAX, elem = 2;
    void *ovf = calloc(nmemb, elem);
    printf("3b) calloc(SIZE_MAX, 2) = %p (%s)\n", ovf,
           ovf == NULL ? "溢出被拦下" : "竟然成功 → 危险");
    free(ovf);

    /* 4) realloc(NULL, n) 等价 malloc(n) */
    int *p = realloc(NULL, 64);
    printf("4) realloc(NULL, 64) = %p (%s)\n", (void *) p,
           p ? "等价 malloc" : "失败");
    p[0] = 7;

    /* 5) realloc 缩容：通常原地，指针不变。
     * 旧地址先存成整数；返回值另起一个变量接，
     * 免得编译器把「realloc 之后还碰旧指针」判成 use-after-free。 */
    uintptr_t b5 = (uintptr_t) p;
    int *q5 = realloc(p, 16);
    if (q5 == NULL) { perror("realloc"); return 1; }
    printf("5) realloc(64->16) 指针%s: %p -> %p\n",
           ((uintptr_t) q5 == b5) ? "不变" : "改变",
           (void *) b5, (void *) q5);
    p = q5;

    /* 6) realloc 扩容：旧数据保留，指针通常变（跨区搬移） */
    for (int i = 0; i < 4; i++) p[i] = i + 1;
    uintptr_t b6 = (uintptr_t) p;
    int *q6 = realloc(p, 1024 * 1024);
    if (q6 == NULL) { perror("realloc"); return 1; }
    printf("6) realloc(16->1MB) 指针%s: %p -> %p; 旧数据 = %d %d %d %d\n",
           ((uintptr_t) q6 == b6) ? "不变" : "改变",
           (void *) b6, (void *) q6, q6[0], q6[1], q6[2], q6[3]);
    p = q6;
    free(p);

    /* 7) realloc(p, 0)：glibc 释放 p 并返回 NULL（此后 p 已成悬垂） */
    void *q = malloc(128);
    void *r = realloc(q, 0);
    printf("7) realloc(p, 0) = %p (%s)\n", r,
           r == NULL ? "glibc：已释放并返回 NULL" : "返回了非 NULL 指针");
    if (r != NULL) free(r);             /* 非 glibc 实现下才需要 */

    /* 8) free(NULL) 是空操作 */
    free(NULL);
    printf("8) free(NULL) 未崩溃\n");
    return 0;
}
