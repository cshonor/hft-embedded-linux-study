# 第 2 章 计算机体系结构与 CPU 工作原理

**Computer Architecture and CPU**

> 🗑️ **本章正文已删除（2026-09-08）**
>
> 原 48 篇（芯片制造、CPU 设计、冯/哈佛架构、Cache、流水线、多核异构、总线 ISA、大小端）
> 全部是 **CSAPP 第 1/3/4/5/6 章讲得更深的内容**，按"别的书讲过的硬件不重复"原则**整章删除**。
> 需要回看时：`git log --diff-filter=D --name-only -- ch02-computer-architecture-and-cpu/` 可找回。
>
> 取舍依据见 [00 · 本书取舍与补写顺序](../00-ROADMAP-本书取舍与补写顺序.md)。

## 本章原本讲什么（已迁移去向）

| 原小节 | 主题 | 去哪了 |
|---|---|---|
| 2.1 芯片诞生 | 硅 → PN 结 → 流片 → 封装 | CSAPP ch1；与 C 语言无关 |
| 2.2 CPU 设计 | 图灵机、RTL、设计流程 | CSAPP ch4 |
| 2.3 体系结构 | 冯·诺依曼 / 哈佛 / 混合 | CSAPP ch1 |
| 2.4 Cache | 局部性、L1/L3、DMA 一致性 | **CSAPP ch6**（讲得更深） |
| 2.5 流水线 | 冒险、分支预测、乱序、SIMD | **CSAPP ch4/5** |
| 2.6 多核 | 互连、big.LITTLE、超线程 | CSAPP ch1/6 |
| 2.7 异构 | GPU/DSP/FPGA/TPU/NPU | 与本书主线无关 |
| 2.8 总线与地址 | MMIO、编址、大小端 | **已改造为 C 语言视角 → [10.8 嵌入式 C 开门](../ch10-multitasking-and-os/10.8-register/10.8-寄存器操作.md)** |
| 2.9 ISA | 指令集 vs 微架构、寻址 | **CSAPP ch3/4**（x86-64，更深） |

## 从本章只带走两条结论（写给 C 程序员）

这两条不是硬件知识，而是**会改变你 C 代码写法的约束**，所以留下来：

| 硬件事实 | 对 C 代码的约束 | 落位 |
|---|---|---|
| cache line 通常是 **64 B**；两个高频写的变量落在同一行会**伪共享** | 热点结构体按 cache line padding / `__attribute__((aligned(64)))` | [ch06 6.7 aligned](../ch06-gnu-c-extensions/6.7-aligned/) |
| 分支预测失败要清空流水线（十几到二十几个周期） | 用 `likely()/unlikely()`（`__builtin_expect`）给编译器提示 | [ch06 6.11 内建函数](../ch06-gnu-c-extensions/6.11-builtin/) |

> 反过来有一条**常见误解**要在这里堵死：**`volatile` 管不了 Cache**。
> 它只约束编译器优化，不产生任何 Cache 维护指令；DMA 一致性要用 `dma_sync_*` / 非缓存映射。
> 实测见 [10.8 第一节](../ch10-multitasking-and-os/10.8-register/10.8-寄存器操作.md)。

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | [ch01 工具链](../ch01-tools-of-the-trade/) |
| 后置 | [ch03.6 C 与汇编混合编程](../ch03-arm-architecture-and-assembly/3.6-mixed-programming/)（内联汇编 = GNU C 扩展，保留）· [ch04 编译链接](../ch04-compile-link-install-run/) · [ch06 GNU C 扩展](../ch06-gnu-c-extensions/) |
