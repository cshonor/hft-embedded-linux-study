/* TLPI 第 2 章 §2.9 —— 静态库与共享库：进程启动时到底加载了什么
 *
 * 编译：gcc -O0 -Wall -Wextra c2_9_shared.c -o c2_9 -ldl
 *      （glibc 2.34+ 里 dlopen/dlsym 已并入 libc，-ldl 只是兼容写法）
 * 运行：./c2_9
 *
 * 本节要钉死的事实：
 *   ① 可执行文件里不存 libc 的代码，只存一个「依赖清单」（DT_NEEDED）。
 *   ② 真正的加载由 ld-linux（动态链接器）在 exec 之后完成，
 *      所以 /proc/self/maps 里能直接看到一排 .so 的映射段。
 *   ③ dlopen/dlsym/dlerror 让程序在运行时才决定加载哪个库、调哪个符号 ——
 *      这是插件体系的底座。
 *   ④ 静态链接把库代码拷进可执行文件：启动快、无 .so 依赖，但体积大、无法共享内存页。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>

/* 收集 /proc/self/maps 里出现的 .so 路径（去重） */
static int collect_sos(char paths[][256], int max)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    int n = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = strchr(line, '/');
        if (!p) continue;
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        if (!strstr(p, ".so")) continue;
        int dup = 0;
        for (int i = 0; i < n; i++)
            if (strcmp(paths[i], p) == 0) { dup = 1; break; }
        if (dup) continue;
        if (n < max) snprintf(paths[n++], 256, "%s", p);
    }
    fclose(f);
    return n;
}

int main(void)
{
    printf("=== ① 程序启动时已经加载了哪些共享库 ===\n");
    static char sos[64][256];
    int n = collect_sos(sos, 64);
    printf("  /proc/self/maps 里出现的 .so（已去重，共 %d 个）：\n", n);
    for (int i = 0; i < n; i++)
        printf("    [%d] %s\n", i, sos[i]);
    printf("  → 这些段在 exec 之后就由动态链接器 ld-linux 建好了，\n");
    printf("     不是程序自己 mmap 的。\n");

    printf("\n  逐个说明（按名字判断，不靠外部命令）：\n");
    for (int i = 0; i < n; i++) {
        const char *role =
            strstr(sos[i], "ld-linux")     ? "动态链接器本体（先跑起来的那个）" :
            strstr(sos[i], "libc.so")      ? "C 运行时：syscall 薄封装、malloc、stdio" :
            strstr(sos[i], "libm.so")      ? "数学库：sqrt/pow/sin…" :
            strstr(sos[i], "libpthread")   ? "线程库（glibc 2.34 起已并入 libc）" :
            strstr(sos[i], "libdl")        ? "dlopen/dlsym（glibc 2.34 起已并入 libc）" :
            strstr(sos[i], "libgcc_s")     ? "GCC 运行时（异常展开、除法辅助）" : "其他";
        printf("    %-46s %s\n", sos[i], role);
    }

    printf("\n  libc 一被加载，malloc 就一定也在：验证一下\n");
    void *m = malloc(16);
    printf("    malloc(16) = %p   %s\n", m, m ? "成功" : "失败");
    free(m);

    printf("\n=== ② 运行时才决定加载谁：dlopen / dlsym / dlerror ===\n");
    printf("  dlopen(\"libm.so.6\", RTLD_NOW) ...\n");
    void *h = dlopen("libm.so.6", RTLD_NOW);
    if (!h) {
        printf("    失败：%s\n", dlerror());
    } else {
        printf("    成功，handle = %p\n", h);

        double (*my_sqrt)(double) = (double (*)(double))dlsym(h, "sqrt");
        const char *err = dlerror();
        if (err) {
            printf("    dlsym(\"sqrt\") 失败：%s\n", err);
        } else {
            printf("    dlsym(\"sqrt\") = %p\n", (void *)my_sqrt);
            printf("    调用 my_sqrt(2.0) = %.15f\n", my_sqrt(2.0));
            printf("    调用 my_sqrt(16.0) = %.15f\n", my_sqrt(16.0));
            printf("    → 符号地址是运行时才拿到的，可执行文件里没有这个地址。\n");
        }

        /* 故意找一个不存在的符号 */
        dlerror();                      /* 先清空错误状态 */
        void *bad = dlsym(h, "no_such_symbol_xyz");
        err = dlerror();
        printf("    dlsym(\"no_such_symbol_xyz\") = %p\n", bad);
        printf("    dlerror() = \"%s\"\n", err ? err : "(null)");
        printf("    → dlsym 返回 NULL 不一定是错（符号值可能真是 0），\n");
        printf("       所以必须用 dlerror() 判错 —— 这是 dl 系列的标准姿势。\n");

        printf("    dlclose(h) = %d\n", dlclose(h));
    }

    printf("\n  再试一个不存在的库，看错误文本：\n");
    void *h2 = dlopen("libdefinitely_not_here.so.99", RTLD_NOW);
    printf("    dlopen(...) = %p\n", h2);
    printf("    dlerror() = \"%s\"\n", dlerror());
    printf("    → 错误文本里通常带「cannot open shared object file」和该库名，\n");
    printf("       排「缺 .so」类线上事故就看这一行。\n");

    printf("\n=== ③ 静态链接 vs 动态链接 ===\n");
    printf("  %-16s %-34s %s\n", "aspect", "dynamic (default)", "static (-static)");
    printf("  %-16s %-34s %s\n", "----------------", "----------------------------------",
           "----------------------------------");
    printf("  %-16s %-34s %s\n", "where is the code", "in separate .so, shared",
           "copied into the executable");
    printf("  %-16s %-34s %s\n", "startup cost", "ld-linux does relocation",
           "almost none");
    printf("  %-16s %-34s %s\n", "deploy deps", "must ship .so (glibc-version sensitive)",
           "single file, no deps");
    printf("  %-16s %-34s %s\n", "binary size", "small", "large");
    printf("  %-16s %-34s %s\n", "upgrade a library", "drop in a new .so (no rebuild)",
           "must rebuild everything");
    printf("  %-16s %-34s %s\n", "memory", "code pages shared across processes",
           "one copy per process");
    printf("\n  嵌入式/HFT 的取舍：\n");
    printf("    · 固件体积敏感、启动时间敏感 → 静态链接（或 musl 静态）；\n");
    printf("    · 要在多个进程间共享几 MB 的库代码页 → 动态链接；\n");
    printf("    · HFT 里更在意的是「首包前的加载抖动」，静态可把这部分确定性拉满。\n");

    printf("\n  静态链接会把 libc 一起塞进来，所以链接命令要显式写 -static：\n");
    printf("    gcc -O2 -static c2_9_shared.c -o c2_9_static\n");
    printf("  之后 file / ldd 会显示 \"not a dynamic executable\"，\n");
    printf("  /proc/self/maps 里也只剩匿名段，看不到 .so。\n");
    return 0;
}
