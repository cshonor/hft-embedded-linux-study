/* T8: PREL32 相对偏移 vs 绝对地址 —— CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
 * 内核在 ARM64/x86-64 上用 32 位相对偏移存放 initcall 指针（省 4 字节/项 + 无重定位）。
 * init.h:
 *   #ifdef CONFIG_HAVE_ARCH_PREL32_RELOCATIONS
 *   typedef int initcall_entry_t;   // 存的是 32 位偏移，不是函数指针
 *   static inline initcall_t initcall_from_entry(initcall_entry_t *entry) {
 *       return offset_to_ptr(entry);  // 编译时计算 偏移 + 指针
 *   }
 *   #else
 *   typedef initcall_t initcall_entry_t;  // 直接存函数指针
 *   #endif
 *
 * 这里用 union 对比两种存储的大小和布局。
 */
#include <stdio.h>
#include <stddef.h>

typedef int (*initcall_t)(void);

static int sample_init(void) { return 0; }

/* 绝对地址版（8 字节/项 on x86-64） */
static initcall_t abs_entry __attribute__((section(".data"))) = sample_init;

/* 相对偏移版（4 字节/项）——用 32 位整数存偏移 */
#include <stdint.h>
/* 注意：真正的 PREL32 用 R_X86_64_PC32 重定位，这里用运行时计算模拟 */
static int32_t rel_offset;

static initcall_t initcall_from_rel(int32_t *entry)
{
    /* 模拟 offset_to_ptr：把偏移加到入口地址 */
    return (initcall_t)((char*)entry + *entry);
}

int main(void)
{
    printf("T8: PREL32 vs absolute\n");
    printf("  sizeof(initcall_t)   = %zu (absolute addr)\n", sizeof(initcall_t));
    printf("  sizeof(int32_t)      = %zu (PREL32 offset)\n", sizeof(int32_t));
    printf("  savings per entry    = %zu bytes\n", sizeof(initcall_t) - sizeof(int32_t));

    /* 模拟 PREL32：计算 sample_init 相对于 rel_offset 的偏移 */
    rel_offset = (int32_t)((char*)sample_init - (char*)&rel_offset);
    printf("\n  rel_offset stored at %p = %d (0x%x)\n",
           (void*)&rel_offset, rel_offset, (unsigned)rel_offset);
    printf("  sample_init at       = %p\n", (void*)sample_init);

    /* 读回 */
    initcall_t recovered = initcall_from_rel(&rel_offset);
    printf("  recovered fn         = %p\n", (void*)recovered);
    printf("  match? %s\n", (recovered == sample_init) ? "YES" : "NO");

    printf("\n  on 1000 initcalls:\n");
    printf("    absolute: %zu bytes\n", (size_t)(1000 * sizeof(initcall_t)));
    printf("    PREL32:   %zu bytes\n", (size_t)(1000 * sizeof(int32_t)));
    printf("    saved:    %zu bytes\n",
           (size_t)(1000 * (sizeof(initcall_t) - sizeof(int32_t))));
    return 0;
}
