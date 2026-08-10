# 6.5 DMB / DSB / ISB 内存屏障（预览）

> 来源：§6.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

DMB/DSB/ISB 三条内存屏障指令的预览——控制内存访问和指令执行顺序。

## 核心要点

| 指令 | 作用 | 强度 |
|------|------|------|
| DMB | 数据内存屏障：保证 DMB 前后的访存有序 | 中 |
| DSB | 数据同步屏障：DMB + 等待所有访存完成 | 强 |
| ISB | 指令同步屏障：冲刷流水线，重新取指 | 最强 |

```asm
str x0, [x1]      ; 写数据
dmb ish           ; 保证写完成
str x2, [x3]      ; 写标志
```

- DMB：排序但不等待
- DSB：排序 + 等待完成（用于 DMA、页表修改等场景）
- ISB：冲刷流水线（用于修改代码、系统寄存器后）

> 详见 Ch18-19 屏障与原子。

## HFT 关联

内存屏障是弱序内存模型下的正确性保障，但有性能代价：
- DMB/DSB 阻止 CPU 乱序优化 → 减少并行度，增加延迟
- HFT 无锁队列需要在正确位置放屏障 → 太少导致数据竞争，太多损害性能
- LDAR/STLR（acquire/release）比显式 DMB 更精确 → 只排序相关访问
- HFT 优先用单线程模型 → 完全不需要屏障

## 自测题

1. DMB 和 DSB 的区别？
<detail><summary>答案</summary>
DMB 保证其前后的访存操作有序，但不等待操作完成——CPU 可以继续执行后续指令。DSB 不仅有 DMB 的排序功能，还等待所有之前的访存操作完成才继续——更强的同步点。
</details>

2. 什么时候必须用 ISB？
<detail><summary>答案</summary>
1. 修改系统寄存器后（如开 MMU 后冲刷流水线中的旧指令）
2. 修改代码后（自修改代码，确保取到新指令）
3. 刷新 I-cache 后
4. 切换执行状态后（如 AArch32↔AArch64）
ISB 冲刷流水线，保证后续指令重新取指。
</details>

3. 为什么 HFT 要尽量减少屏障指令？
<detail><summary>答案</summary>
屏障指令阻止 CPU 乱序执行和访存合并优化，降低 IPC 和并行度。每条 DMB/DSB 可能增加数个周期的延迟。HFT 热路径中多余的屏障会累积延迟。用 LDAR/STLR 精确替代、用单线程避免共享、用 per-CPU 数据避免屏障。
</details>

## 参考与延伸

- 原书 §6.5
- [Ch18 内存屏障详解](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
- [Ch19 屏障使用案例](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md)
