# 10.3 常用实战示例

> 来源：§10.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

内核和裸机开发中常用的内联汇编模式：系统寄存器读写、屏障、原子操作、延迟、字符串操作。

## 1. 读/写系统寄存器

```c
/* 读系统寄存器 */
#define read_sysreg(reg) ({                                   \
    u64 val;                                                   \
    asm volatile("mrs %0, " __stringify(reg) : "=r"(val));    \
    val;                                                       \
})

/* 写系统寄存器 */
#define write_sysreg(val, reg)                                \
    asm volatile("msr " __stringify(reg) ", %0" : : "r"(val))

/* 使用示例 */
u64 el = read_sysreg(CurrentEL) >> 2;   /* 读当前异常等级 */
write_sysreg(0x1, DAIF);                 /* 屏蔽中断 */
u64 t = read_sysreg(cntvct_el0);        /* 读虚拟计时器 */
```

| 系统寄存器 | 用途 |
|-----------|------|
| CurrentEL | 当前异常等级（EL0-EL3） |
| DAIF | 中断屏蔽标志 |
| cntvct_el0 | 虚拟计时器计数值（时间戳） |
| SCTLR_EL1 | 系统控制（MMU/Cache 开关） |
| TTBR0_EL1 | 用户页表基址 |
| VBAR_EL1 | 异常向量基址 |

### __stringify 宏

```c
/* 把宏参数转为字符串 */
#define __stringify_1(x) #x
#define __stringify(x)   __stringify_1(x)

/* __stringify(CurrentEL) → "CurrentEL" */
/* asm("mrs %0, CurrentEL") */
```

## 2. 内存屏障

```c
/* 编译器 + CPU 屏障 */
#define dmb(opt)  asm volatile("dmb " #opt ::: "memory")
#define dsb(opt)  asm volatile("dsb " #opt ::: "memory")
#define isb()     asm volatile("isb" ::: "memory")

/* 编译器屏障（不生成 CPU 指令） */
#define barrier() asm volatile("" ::: "memory")

/* 使用 */
dmb(ish);     /* 内域共享屏障 */
dsb(sy);      /* 全系统屏障 */
isb();        /* 指令同步屏障 */
```

| 屏障 | 作用 | C 对应 |
|------|------|--------|
| `dmb` | 数据内存屏障（后续内存访问在 DMB 前完成后执行） | `smp_mb()` |
| `dsb` | 数据同步屏障（后续所有指令在 DSB 前完成后才执行） | `smp_mb()` + 等待 |
| `isb` | 指令同步屏障（刷新流水线，重取指令） | 无 C 对应 |
| `barrier()` | 编译器屏障（阻止编译器重排，不生成指令） | `barrier()` |

## 3. 原子比较交换（CAS）

```c
/* LDXR/STXR 实现的 cmpxchg */
static inline int cmpxchg(volatile int *ptr, int old, int new)
{
    int oldval, ret;
    asm volatile(
        "1: ldxr %w[old], [%[ptr]]\n"        /* 独占读 */
        "   cmp %w[old], %w[expected]\n"     /* 比较 */
        "   b.ne 2f\n"                       /* 不等 → 跳出 */
        "   stxr %w[ret], %w[new], [%[ptr]]\n" /* 独占写 */
        "   cbnz %w[ret], 1b\n"              /* 写失败 → 重试 */
        "2:\n"
        : [old] "=&r"(oldval), [ret] "=&r"(ret)
        : [ptr] "r"(ptr), [expected] "r"(old), [new] "r"(new)
        : "cc", "memory"
    );
    return oldval;
}
```

### 关键点分析

| 行 | 指令 | 说明 |
|----|------|------|
| LDXR | 独占读 | 启动独占监视器，标记 [ptr] |
| CMP | 比较 | 如果 old != expected → 不写，跳出 |
| STXR | 独占写 | 如果监视器仍有效 → 写成功(ret=0)；否则失败(ret=1) |
| CBNZ | 条件跳转 | ret≠0 → 写失败，跳回重试 |
| `"=&r"` | early clobber | 输出寄存器在输入使用前被写 |
| `"cc"` | 条件标志 | CMP 修改 NZCV |
| `"memory"` | 内存 clobber | 原子操作有内存副作用 |

## 4. 精确延迟

```c
/* 忙等待延迟（纳秒级精度） */
static inline void delay_ns(unsigned long ns)
{
    u64 start, end;
    start = read_sysreg(cntvct_el0);
    /* CNTVCT_EL0 频率通常 = CNTFRQ_EL0（如 100MHz = 10ns/tick） */
    end = start + ns / 10;
    while (read_sysreg(cntvct_el0) < end)
        ;
}
```

## 5. 读取 CPU ID

```c
/* 读取 MPIDR_EL1 获取 CPU ID */
static inline int get_cpu_id(void)
{
    u64 mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    return mpidr & 0xff;     /* Aff0 = CPU ID */
}
```

## 6. 自旋锁

```c
/* 简化版自旋锁加锁 */
static inline void spin_lock(volatile int *lock)
{
    asm volatile(
        "1: wfe\n"                    /* 等待事件（低功耗等待） */
        "   ldaxr %w[tmp], [%[lock]]\n" /* 独占读（acquire语义） */
        "   cbnz %w[tmp], 1b\n"       /* 锁被占 → 继续等 */
        "   stxr %w[tmp], %w[one], [%[lock]]\n" /* 独占写 */
        "   cbnz %w[tmp], 1b\n"       /* 写失败 → 重试 */
        : [tmp] "=&r"(tmp)
        : [lock] "r"(lock), [one] "r"(1)
        : "memory"
    );
}
```

## 7. NOP 序列（对齐优化）

```c
/* 插入 NOP 实现指令对齐 */
#define NOP() asm volatile("nop")
#define NOP5() asm volatile("nop\nnop\nnop\nnop\nnop")

/* 在热循环前插入 NOP 对齐到 cache line */
NOP5();
while (--count > 0) {
    process_item();
}
```

## HFT 关联

- **read_sysreg(cntvct_el0)** → 精确延迟测量，纳秒级时间戳
- **dmb/dsb → 无锁数据结构的内存序保证** → 生产者/消费者队列
- **cmpxchg → 无锁队列的原子操作** → MPSC 队列、环形缓冲区
- **wfe → 低功耗自旋等待** → 空转时降功耗（但仍非 HFT 友好，HFT 倾向忙等）
- **NOP 对齐** → 热循环入口对齐到 cache line 边界

## 自测题

1. memset 内联汇编中为什么 `"memory"` 在 clobber 列表？
<details><summary>答案</summary>
告诉编译器汇编可能修改内存（写入目标区域），不能把前后的内存访问重排。如果不加 `"memory"`，编译器可能把 memset 之后的内存读操作重排到 memset 之前 → 读到旧值。`"memory"` 是编译器层面的内存屏障。
</details>

2. `__stringify(reg)` 的作用是什么？为什么不直接写寄存器名？
<details><summary>答案</summary>
`__stringify` 把宏参数转为字符串字面量。`__stringify(SCTLR_EL1)` → `"SCTLR_EL1"`。不直接写的原因：read_sysreg/write_sysreg 是宏，参数 reg 是宏参数不是字符串，需要 `#` 运算符转字符串。这样 `read_sysreg(SCTLR_EL1)` 展开为 `asm("mrs %0, SCTLR_EL1")`。
</details>

3. cmpxchg 中 `b.ne 2f` 和 `cbnz w3, 1b` 的 `f`/`b` 含义？
<details><summary>答案</summary>
`f` = forward（向前找最近的标号），`b` = backward（向后找最近的标号）。`2f` 跳到前方的标号 2（退出循环），`1b` 跳回后方的标号 1（重试循环）。这是 GNU as 的局部标号机制，同一文件可重复使用数字标号。
</details>

4. cmpxchg 函数中为什么用 `"=&r"` 而不是 `"=r"`？
<details><summary>答案</summary>
`&` 是 early clobber 修饰符。LDXR 写入 oldval 后，后续的 CMP 指令还需要读 expected 输入。如果用 `"=r"`，编译器可能让 oldval 和 expected 共用同一寄存器 → LDXR 后 expected 被覆盖 → CMP 比较错误。`"=&r"` 强制 oldval 独占寄存器。
</details>

5. 为什么自旋锁用 WFE 而不是忙等？
<details><summary>答案</summary>
WFE（Wait For Event）让 CPU 进入低功耗状态，等待其他 CPU 发送 SEV（Send Event）唤醒。减少锁争用时的功耗和总线带宽。但 HFT 场景中可能优先用忙等（WFE 有唤醒延迟），牺牲功耗换取确定延迟。
</details>

## 参考与延伸

- 原书 §10.3
- [10.4 clobber 列表](04-clobber.md)
- [Ch20 原子操作](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)
