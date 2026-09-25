/* startup.c —— 用 C 写启动代码（不写汇编也能起）
 *
 * 操作系统缺席时，C 运行环境的四条前置条件没人替你准备，
 * 只能在这里自己满足：
 *   1. 栈指针 SP  —— 向量表第 0 个字，上电时硬件自动装进 SP
 *   2. 复位入口 PC —— 向量表第 1 个字，上电时硬件自动装进 PC
 *   3. .data 初值  —— 从 Flash 拷到 RAM
 *   4. .bss 清零   —— 全局/静态变量默认 0
 * 少第 3、4 条：`static int x = 5;` 的初值还在 Flash 里，RAM 里是垃圾。
 */

typedef unsigned int u32;

extern u32 _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* ---- 没有 libc，但编译器仍可能替你生成对 memset/memcpy 的调用 -------
 * freestanding 下标准库不参与链接，这些符号必须自己提供，
 * 否则报 undefined symbol（实测：把结构体赋值写进代码，立刻报
 * undefined symbol: __aeabi_memcpy —— 见 libc-trap.c / make check-libc）。
 *
 * ⚠ ARM EABI 的名字与参数顺序和 libc 不一样：
 *   __aeabi_memcpy(dst, src, n)   与 memcpy 同序
 *   __aeabi_memset(dst, n, c)     参数顺序与 memset(dst, c, n) 相反！
 * 编译器生成的调用是 __aeabi_* 而不是 memcpy —— 只提供 memcpy 是链不上的。
 */
void *memset(void *dst, int c, u32 n)
{
    unsigned char *p = (unsigned char *)dst;
    while (n != 0u) {
        *p = (unsigned char)c;
        p = p + 1;
        n = n - 1u;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, u32 n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n != 0u) {
        *d = *s;
        d = d + 1;
        s = s + 1;
        n = n - 1u;
    }
    return dst;
}

/* ARM EABI 别名：编译器内部生成的调用落到这几个名字上 */
void __aeabi_memcpy(void *dst, const void *src, u32 n)
{
    (void)memcpy(dst, src, n);
}

void __aeabi_memcpy4(void *dst, const void *src, u32 n)
{
    (void)memcpy(dst, src, n);
}

void __aeabi_memcpy8(void *dst, const void *src, u32 n)
{
    (void)memcpy(dst, src, n);
}

void __aeabi_memset(void *dst, u32 n, int c)   /* 注意：n 在前、c 在后 */
{
    (void)memset(dst, c, n);
}

void __aeabi_memset4(void *dst, u32 n, int c)
{
    (void)memset(dst, c, n);
}

/* ---- 向量表 ----------------------------------------------------------
 * Cortex-M 的系统异常向量就 0..15，16 之后是厂商外设中断。
 * section(".vectors") + 链接脚本把它钉在 Flash 首地址 0x0800_0000 ——
 * 这个位置是硬件写死的，改不了。
 */
__attribute__((used, section(".vectors")))
const void *const g_vectors[16] = {
    (const void *)&_estack,       /* 0  MSP 初值 */
    Reset_Handler,                /* 1  复位 */
    Default_Handler,              /* 2  NMI */
    Default_Handler,              /* 3  HardFault */
    Default_Handler,              /* 4  MemManage */
    Default_Handler,              /* 5  BusFault */
    Default_Handler,              /* 6  UsageFault */
    0, 0, 0, 0,                   /* 7..10 保留 */
    Default_Handler,              /* 11 SVCall */
    Default_Handler,              /* 12 DebugMonitor */
    0,                            /* 13 保留 */
    Default_Handler,              /* 14 PendSV */
    Default_Handler,              /* 15 SysTick */
};

/* ---- 复位入口：crt0 干的活，自己写 -----------------------------------
 * 顺序不能换：先搬 .data / 清 .bss，再进 main。
 * 反了的话，main 里假定为 0 的全局变量会是 RAM 的随机残留。
 */
void Reset_Handler(void)
{
    u32 *src = &_sidata;
    u32 *dst;

    for (dst = &_sdata; dst < &_edata; ) {
        *dst = *src;
        dst = dst + 1;
        src = src + 1;
    }
    for (dst = &_sbss; dst < &_ebss; ) {
        *dst = 0u;
        dst = dst + 1;
    }

    main();

    for (;;) {
        /* main 不该返回；真返回了就停在这里 */
    }
}

/* 所有未处理的中断都落到这里：裸机阶段先停住，比乱跑好定位 */
void Default_Handler(void)
{
    for (;;) {
        /* 等调试器 attach */
    }
}
