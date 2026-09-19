/*
 * T5 被加载的“模块”：t5_module.c
 *
 * 这就是一个最小的内核模块缩影：
 *   - kernel_print 是未决符号（extern），对应模块引用的内核导出函数
 *   - module_entry 是入口（对应 init_module）
 *
 * 编译成可重定位 .o（-c，不链接）：
 *   gcc -O0 -c t5_module.c -o t5_module.o
 *
 * .o 里的 CALL kernel_print 会有 R_X86_64_PLT32 重定位，
 * 符号 kernel_print 是 SHN_UNDEF（未决）。
 * 加载器要把它 resolve 成自己的 kernel_print 地址并 patch CALL。
 */
extern void kernel_print(void);

int module_entry(void)
{
    kernel_print();   /* 未决调用 —— 加载器来 patch */
    return 42;
}
