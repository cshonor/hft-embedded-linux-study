/* main.c —— 裸机 C 操作硬件的正反面教材
 *
 * 前提：没有操作系统、没有 arm-none-eabi-gcc、没有 libc、没有 CMSIS。
 * 只用 C 语言本身 + 一张地址表（STM32F103 参考手册 RM0008）。
 *
 * 编译：clang --target=armv7m-none-eabi -mcpu=cortex-m3 -mthumb -ffreestanding ...
 * 链接：ld.lld -T linker.ld
 */

typedef unsigned int  u32;
typedef signed   int  i32;
typedef unsigned char u8;

/* ---- 地址表：手册里抄下来的，就这么几个数 ----------------------------
 * 所有外设在 0x4000_0000 往上；每个外设是一段窗口，窗口内是它的寄存器。
 * C 语言不认识"GPIO"，它只认识"往某个地址写 32 位"。
 */
#define RCC_BASE      0x40021000u
#define GPIOC_BASE    0x40011000u

#define RCC_APB2ENR   (*(volatile u32 *)(RCC_BASE   + 0x18u))  /* 外设时钟使能 */
#define GPIOC_CRH     (*(volatile u32 *)(GPIOC_BASE + 0x04u))  /* 端口配置高 8 位 */
#define GPIOC_IDR     (*(volatile u32 *)(GPIOC_BASE + 0x08u))  /* 输入数据 */
#define GPIOC_ODR     (*(volatile u32 *)(GPIOC_BASE + 0x0Cu))  /* 输出数据 */
#define GPIOC_BSRR    (*(volatile u32 *)(GPIOC_BASE + 0x10u))  /* 位设置/清除（原子） */

/* ---- 全局变量的两种命运（链接脚本 + startup.c 在这里被验证）---------
 * g_has_init：有初值，属于 .data。初值 0xA5A5A5A5 躺在 Flash 里，
 *             上电后由 Reset_Handler 拷进 RAM —— 不拷就是垃圾。
 * g_tick    ：无初值，属于 .bss。C 标准要求为 0，靠 Reset_Handler 清。
 *
 * 为什么两个都写 volatile：-Os 下编译器发现"这俩只写不读"，
 * 会把它们连同判断一起优化掉（实测：不加 volatile 时 .data/.bss 都是 0 字节，
 * 链接器 --gc-sections 顺手把段丢了）。volatile 在这里的作用是"让硬件看得见"。
 */
volatile u32 g_has_init = 0xA5A5A5A5u;
volatile u32 g_tick;

/* ---- 反面教材：指针丢掉 volatile ------------------------------------
 * 陷阱：编译器只保证"对 volatile 对象的访问"不可合并、不可删除。
 * 普通指针读的是内存，它有权认为"这块内存没人改过"，于是把读搬到循环外。
 * 表现：按键永远等不到——因为 IDR 只被读了一次。
 */
__attribute__((used, noinline))
u32 wait_press_bad(void)
{
    u32 *idr = (u32 *)GPIOC_BASE + 0x08u / 4u;   /* 普通指针：编译器可优化 */
    while ((*idr & (1u << 13)) == 0u) {
        /* 本意是轮询 PC13，实际循环体里没有 load */
    }
    return 1u;
}

/* ---- 正面：volatile 指针 ---------------------------------------------
 * volatile 在这里不是"线程安全"，而是"C 语言里唯一能表达
 * '这个地址的内容会被硬件改变' 的手段"。
 */
__attribute__((used, noinline))
u32 wait_press_good(void)
{
    volatile u32 *idr = (volatile u32 *)(GPIOC_BASE + 0x08u);
    while ((*idr & (1u << 13)) == 0u) {
        /* 每轮重新 load，循环体里一定有 ldr */
    }
    return 1u;
}

/* 软件延时：参数必须是 volatile，否则 while(n--) 会被优化成 while(1) */
static void delay(volatile u32 n)
{
    while (n != 0u) {
        n = n - 1u;
    }
}

int main(void)
{
    /* 1. 先开时钟：F1 上不使能外设时钟，寄存器写进去等于扔进垃圾桶 */
    RCC_APB2ENR |= (1u << 4);            /* IOPCEN */

    /* 2. 配置 PC13 为通用推挽输出、2MHz：清 4 位再置值 */
    GPIOC_CRH &= ~(0xFu << 20);
    GPIOC_CRH |=  (0x2u << 20);

    /* 3. 主循环：BSRR 写 1 置位、写 1<<16 复位（读改写用 ODR 会被中断撕） */
    for (;;) {
        if (g_has_init != 0xA5A5A5A5u) {   /* 让 .data 真正被引用，别被 --gc-sections 丢掉 */
            GPIOC_ODR = g_has_init;         /* .data 没搬运成功时的"证据灯" */
        }
        g_tick = g_tick + 1u;               /* 让 .bss 真正被引用 */
        GPIOC_BSRR = (1u << 13);            /* PC13 = 1 → 灭 */
        delay(400000u);
        GPIOC_BSRR = (1u << (13u + 16u));   /* PC13 = 0 → 亮 */
        delay(400000u);
    }
}
