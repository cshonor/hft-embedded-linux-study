/* Ch7 §7.1 — 对齐分配：malloc 的默认对齐 vs posix_memalign / aligned_alloc
 * 编译: gcc -O2 -Wall -Wextra -o c7_5_memalign c7_5_memalign.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

int main(void)
{
    /* malloc 保证的对齐 = max_align_t 的对齐（x86-64 上为 16） */
    printf("_Alignof(max_align_t) = %zu\n\n", _Alignof(max_align_t));

    /* 连续 8 次 malloc(1)，看地址低 4 位 */
    printf("malloc(1) 连续 8 次的地址（低 4 位应恒为 0）:\n");
    void *ps[8];
    for (int i = 0; i < 8; i++) {
        ps[i] = malloc(1);
        printf("   %p   低4位 = %zu\n", ps[i], (uintptr_t) ps[i] & 0xf);
    }
    for (int i = 0; i < 8; i++) free(ps[i]);

    /* posix_memalign：成功返回 0，失败返回 errno 值（不是设 errno） */
    size_t align = 4096;
    void *pg = NULL;
    int rc = posix_memalign(&pg, align, 100);
    if (rc != 0) { fprintf(stderr, "posix_memalign: %s\n", strerror(rc)); return 1; }
    printf("\nposix_memalign(4096, 100) = %p,  %% 4096 = %zu\n",
           pg, (uintptr_t) pg % align);
    free(pg);

    /* 非法对齐（不是 2 的幂）→ 返回 EINVAL，不是崩溃 */
    void *bad = NULL;
    rc = posix_memalign(&bad, 100, 64);
    printf("posix_memalign(100, 64) rc = %d (%s)\n", rc,
           rc ? "EINVAL，被拒绝" : "竟然成功");
    free(bad);

    /* aligned_alloc (C11)：要求 size 是 align 的整数倍 */
    void *av = aligned_alloc(64, 64 * 4);
    printf("aligned_alloc(64, 256) = %p,  %% 64 = %zu\n",
           av, (uintptr_t) av % 64);
    free(av);
    return 0;
}
