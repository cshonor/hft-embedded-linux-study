/*
 * T1: EXPORT_SYMBOL 的 section 收集 + bsearch 查找 + CRC
 *
 * 模拟内核 v6.6 的导出符号机制：
 *   - K_EXPORT(fn) 把一个 struct ksym 放进 my_ksymtab 段
 *   - GNU ld 自动提供 __start_my_ksymtab / __stop_my_ksymtab 边界符号
 *   - 加载时 qsort 排序，查找时 bsearch —— 跟内核 find_symbol 一模一样
 *
 * 设计要点：struct ksym 只放 name + value（16 字节），CRC 单独算 —— 这正是
 *   内核的做法：__ksymtab 存 struct kernel_symbol{name,value}，
 *   __kcrctab 单独存 s32 crc 数组（一一对应）。
 *   让 sizeof(ksym)=16 == 段步长，避免段对齐填充导致计数错位。
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* 运行时的导出符号表条目（对应 struct kernel_symbol 的直接指针形态） */
struct ksym {
    const char *name;       /* 符号名 */
    void       *value;      /* 符号地址 */
    /* crc 不放在这里：内核把 crc 单独放 __kcrctab 段，这里也分开算 */
};

/* 简化版 CRC：对符号名求一个 hash，模拟 genksyms 的输出 */
static long fake_crc(const char *s)
{
    long h = 0xcbf29ce484222325UL;
    for (; *s; s++)
        h = (h ^ (unsigned char)*s) * 0x100000001b3UL;
    return (long)(h & 0xffffffffL);
}

/*
 * K_EXPORT(fn) —— 对应内核 EXPORT_SYMBOL(fn)
 *   内核源码：extern typeof(fn) fn; __ADDRESSABLE(fn);
 *             asm(".section .export_symbol ... .quad fn")
 *   这里直接用 C 级 __attribute__((section))，跳过 asm 中间态
 * 注：sizeof(struct ksym)=16，段对齐 16，步长==sizeof，无填充。
 */
#define K_EXPORT(fn) \
    static const struct ksym __ksym_##fn \
        __attribute__((used, section("my_ksymtab"))) = \
        { #fn, (void *)(fn) }

#define K_EXPORT_GPL(fn) \
    static const struct ksym __ksym_gpl_##fn \
        __attribute__((used, section("my_ksymtab_gpl"))) = \
        { #fn, (void *)(fn) }

/* 被导出的“内核函数” */
static void printk_core(const char *msg)  { printf("[kernel] %s\n", msg); }
static int  kmalloc_core(unsigned n)       { return (int)(long)malloc(n ? n : 1); }
static void kfree_core(void *p)           { free(p); (void)p; }
static void schedule_core(void)           { printf("[kernel] schedule\n"); }
static void gpl_only_helper(void)         { printf("[kernel] gpl-only helper\n"); }

K_EXPORT(printk_core);
K_EXPORT(kmalloc_core);
K_EXPORT(kfree_core);
K_EXPORT(schedule_core);
K_EXPORT_GPL(gpl_only_helper);

/* GNU ld 对名字是合法 C 标识符的 section 自动生成边界符号 */
extern const struct ksym __start_my_ksymtab[];
extern const struct ksym __stop_my_ksymtab[];
extern const struct ksym __start_my_ksymtab_gpl[];
extern const struct ksym __stop_my_ksymtab_gpl[];

/* 比较函数：qsort 和 bsearch 共用，两边都是 struct ksym 指针 */
static int cmp_ksym(const void *pa, const void *pb)
{
    const struct ksym *a = pa, *b = pb;
    return strcmp(a->name, b->name);
}

/* 对应内核 find_exported_symbol_in_section：bsearch */
static const struct ksym *find_in_table(const struct ksym *start,
                                        const struct ksym *stop,
                                        const char *name)
{
    size_t n = stop - start;
    const struct ksym key = { .name = name };
    return bsearch(&key, start, n, sizeof(struct ksym), cmp_ksym);
}

/* 对应内核 find_symbol：先查普通表，再查 GPL 表。
 * 注意：bsearch 要求表已排序，所以查的是 main() 里排好序的副本
 * （内核启动时也会先排序 __ksymtab） */
static struct ksym *g_norm;
static struct ksym *g_gpl;
static size_t       g_nn, g_ng;

static const struct ksym *find_symbol(const char *name, int gplok)
{
    const struct ksym *s;
    if (g_nn && (s = find_in_table(g_norm, g_norm + g_nn, name)))
        return s;
    if (gplok && g_ng && (s = find_in_table(g_gpl, g_gpl + g_ng, name)))
        return s;
    return NULL;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    size_t n_norm = __stop_my_ksymtab - __start_my_ksymtab;
    size_t n_gpl  = __stop_my_ksymtab_gpl - __start_my_ksymtab_gpl;

    printf("=== T1: EXPORT_SYMBOL section 收集 ===\n");
    printf("my_ksymtab       收集到 %zu 个符号（NOT_GPL_ONLY）\n", n_norm);
    printf("my_ksymtab_gpl   收集到 %zu 个符号（GPL_ONLY）\n", n_gpl);
    printf("sizeof(struct ksym) = %zu（name+value，crc 分离）\n\n",
           sizeof(struct ksym));

    /* 加载时排序，模拟内核启动时排序 __ksymtab */
    g_norm = malloc(n_norm * sizeof(*g_norm));
    g_gpl  = malloc(n_gpl  * sizeof(*g_gpl));
    g_nn = n_norm; g_ng = n_gpl;
    if (n_norm) memcpy(g_norm, __start_my_ksymtab, n_norm * sizeof(*g_norm));
    if (n_gpl)  memcpy(g_gpl,  __start_my_ksymtab_gpl, n_gpl  * sizeof(*g_gpl));
    qsort(g_norm, n_norm, sizeof(*g_norm), cmp_ksym);
    qsort(g_gpl,  n_gpl,  sizeof(*g_gpl),  cmp_ksym);
    struct ksym *norm = g_norm;
    struct ksym *gpl  = g_gpl;

    printf("--- 排序后的普通导出表（对应排序后的 __ksymtab） ---\n");
    for (size_t i = 0; i < n_norm; i++)
        printf("  [%zu] %-16s @ %p  crc=0x%08lx\n",
               i, norm[i].name, norm[i].value, fake_crc(norm[i].name));
    printf("--- 排序后的 GPL 导出表（对应 __ksymtab_gpl） ---\n");
    for (size_t i = 0; i < n_gpl; i++)
        printf("  [%zu] %-16s @ %p  crc=0x%08lx\n",
               i, gpl[i].name, gpl[i].value, fake_crc(gpl[i].name));

    /* 用 bsearch 查找（内核 find_symbol 的用户态版） */
    printf("\n=== bsearch 查找（模拟模块 resolve_symbol） ===\n");
    const char *queries[] = {"printk_core", "schedule_core",
                             "gpl_only_helper", "missing_symbol"};
    for (size_t i = 0; i < sizeof(queries)/sizeof(queries[0]); i++) {
        const struct ksym *s = find_symbol(queries[i], 1 /* gplok */);
        if (s)
            printf("  resolve %-16s -> %p  crc=0x%08lx\n",
                   s->name, s->value, fake_crc(s->name));
        else
            printf("  resolve %-16s -> NOT FOUND (ENOENT)\n", queries[i]);
    }

    /* 非 GPL 模块查 GPL-only 符号：被拒绝（内核 license 检查） */
    printf("\n=== license 检查：非 GPL 模块查 GPL-only 符号 ===\n");
    const struct ksym *g = find_symbol("gpl_only_helper", 0 /* gplok=0 */);
    printf("  非 GPL 模块查 gpl_only_helper -> %s\n",
           g ? "LEAKED（错误！）" : "拒绝（GPL_ONLY）");

    /* 真正调用查到的函数：模块通过决议后的符号调用内核函数 */
    printf("\n=== 通过决议的符号调用内核函数 ===\n");
    const struct ksym *pk = find_symbol("printk_core", 1);
    if (pk) {
        void (*fn)(const char *) = (void (*)(const char *))pk->value;
        fn("hello from resolved printk_core");
    }
    const struct ksym *km = find_symbol("kmalloc_core", 1);
    const struct ksym *kf = find_symbol("kfree_core", 1);
    if (km && kf) {
        int *(*alloc)(unsigned) = (int *(*)(unsigned))km->value;
        void  (*free)(void*)    = (void  (*)(void*))   kf->value;
        int *p = (int *)(long)alloc(16);
        printf("  kmalloc -> %p, kfree 回收\n", (void*)p);
        free(p);
        (void)free; (void)alloc;
    }

    free(norm);
    free(gpl);
    return 0;
}
