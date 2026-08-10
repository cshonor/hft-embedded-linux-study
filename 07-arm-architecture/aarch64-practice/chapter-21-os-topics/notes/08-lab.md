# §21.8 实验要点

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch21 的实验通过在 BenOS 上逐步实现进程管理，将前面学到的 A64 指令、异常、中断知识综合运用到操作系统实践中。

## 核心要点

### 实验列表

| 实验 | 内容 | 平台 | 核心知识点 |
|------|------|------|-----------|
| 21-1 | 观察栈布局（GDB 看栈帧） | QEMU | AAPCS64 栈帧、FP/LR |
| 21-2 | 进程创建（do_fork） | QEMU | PCB、栈设计 |
| 21-3 | 进程调度（轮转 + 定时器） | QEMU | schedule、switch_to |
| 21-4 | 新增 malloc 系统调用 | QEMU | SVC、syscall 分发 |
| 21-5 | 新增 clone 系统调用 | QEMU | fork + syscall |

### 实验 21-1 要点

用 GDB 在函数入口设断点，观察栈帧布局：
- `info registers sp fp x30` — 查看 SP/FP/LR
- `x/16gx $sp` — 查看栈内容
- 验证 STP X29, X30 是否正确保存了 FP/LR

### 实验 21-3 要点

```
定时器中断 → timer_irq_handler → schedule → switch_to → 新进程运行
                                                ↓
                                          新进程打印 PID
                                                ↓
                                          下一次定时器中断 → 再调度
```

## HFT 关联

这些实验是理解操作系统调度的基础。HFT 开发者需要理解：(1) 实验 21-1 的栈帧分析技巧在调试 crash 时直接使用（`gdb` 的 `bt` 或 `crash` 工具）；(2) 实验 21-3 的调度机制帮助理解为什么交易线程会被抢占以及如何避免；(3) 实验 21-4 的 syscall 开销测量是 HFT 性能优化的基本技能。

## 自测题

1. **在 GDB 中如何查看当前函数的栈帧？FP 指向什么？**

<details>
<summary>答案</summary>

用 `info frame` 查看当前栈帧，`info registers fp sp x30` 查看 FP/SP/LR。FP(X29) 指向栈帧中保存的**上一个 FP 的地址**——即调用者的 FP。通过 `x/gx $fp` 可以读到上一个 FP，`x/gx $fp+8` 可以读到调用者的返回地址(LR)。`backtrace` 命令自动沿 FP 链表遍历打印调用栈。
</details>

2. **实验 21-3 中，如果定时器中断频率设得太低会怎样？太高呢？**

<details>
<summary>答案</summary>

**太低**：每个进程获得很长的执行时间，看起来像顺序执行而非并发——响应延迟高。**太高**：频繁的上下文切换开销占比过大，实际有效工作时间减少（thrashing）。BenOS 课堂实验中一般设 100-1000Hz。Linux 默认 250Hz（CONFIG_HZ=250），实时内核可选 1000Hz。HFT 系统反而用更低频率或 NOHZ_FULL 关闭调度时钟——因为交易线程不需要被抢占，频繁的定时器中断反而是干扰。
</details>

## 参考与延伸

- [§21.9 易错点清单](09-pitfalls.md) — 实验中常见错误
- [Ch12 中断处理](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — 定时器中断配置
