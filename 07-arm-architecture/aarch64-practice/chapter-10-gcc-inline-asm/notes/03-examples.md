# 10.3 常用实战示例

> 来源：§10.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

内核中常用的内联汇编示例。

## 核心要点

```c
// 读系统寄存器
u64 val;
asm volatile("mrs %0, CurrentEL" : "=r"(val));

// 屏障
#define dmb(opt) asm volatile("dmb " #opt ::: "memory")

// 原子比较交换
asm volatile(
    "1: ldxr %w0, [%1]\n"
    "   cmp %w0, %w2\n"
    "   b.ne 2f\n"
    "   stxr w3, %w2, [%1]\n"
    "   cbnz w3, 1b\n"
    "2:"
    : "=&r"(oldval)
    : "r"(ptr), "r"(old), "r"(new)
    : "cc", "memory"
);
```

## HFT 关联

- read_sysreg → 读取 cntvct_el0 做延迟测量
- dmb/dsb → 无锁数据结构的内存序保证
- cmpxchg → 无锁队列的原子操作

## 自测题

1. memset 内联汇编中为什么 `"memory"` 在 clobber 列表？
<details><summary>答案</summary>
告诉编译器汇编可能修改内存，不能把前后的内存访问重排。这是编译器层面的内存屏障。
</details>

2. `__stringify(reg)` 的作用？
<details><summary>答案</summary>
宏展开时把参数转为字符串。`__stringify(SCTLR_EL1)` → `"SCTLR_EL1"`。
</details>

3. cmpxchg 中 `b.ne 2f` 和 `cbnz w3, 1b` 的 `f`/`b` 含义？
<details><summary>答案</summary>
`f` = forward（前方标号），`b` = backward（后方标号）。`2f` 跳到前方的标号 2，`1b` 跳回后方的标号 1。
</details>

## 参考与延伸

- 原书 §10.3
- [Ch20 原子操作](../../chapter-20-atomic-operations/notes/section-0-本章完整概述.md)
