# §20.4 WFE / SEV —— 低功耗自旋锁

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

WFE（Wait For Event）让 CPU 进入低功耗等待，SEV（Send Event）唤醒等待的 CPU。WFE 自旋锁比普通自旋锁省电，减少总线竞争。但 WFE 可能被误唤醒，醒来后必须重新检查条件。

## 核心要点

### WFE / SEV 指令

| 指令 | 行为 |
|------|------|
| `WFE` | Wait For Event：进入低功耗，等事件唤醒 |
| `SEV` | Send Event：唤醒所有等 WFE 的 CPU |
| `SEVL` | Send Event Local：只唤醒本核（用于 WFE 前预发） |

### WFE 自旋锁

```asm
; 低功耗自旋锁
spin_lock_wfe:
1:  ldxr w1, [x0]       ; 读锁
    cbnz w1, 2f          ; 锁被占 → 等待
    stxr w2, w1, [x0]    ; 尝试获取（w1=1）
    cbnz w2, 1b          ; STXR 失败 → 重试
    ret                   ; 获取成功

2:  wfe                   ; 低功耗等待
    b 1b                  ; 被唤醒后重试

spin_unlock_wfe:
    stlr w1, [x0]        ; 释放锁（STLR 自带 release 屏障）
    sev                   ; 唤醒等待的 CPU
    ret
```

### WFE vs 普通自旋

| 特性 | 普通自旋 | WFE 自旋 |
|------|----------|----------|
| 等待时 CPU 状态 | 100% 占用 | 低功耗 |
| 总线竞争 | 高（反复 LDXR） | 低（睡眠等待） |
| 唤醒延迟 | 即时（下一轮循环） | ~10-50ns（WFE 唤醒） |
| 误唤醒 | 无 | 有（中断等可能误唤醒） |

> **WFE 优势**：自旋时不用 100% 占 CPU，降低功耗。
> **WFE 陷阱**：WFE 可能被其他事件（中断、调试）误唤醒，醒来后要重新检查条件。

## HFT 关联

WFE 自旋锁在 HFT 中有双重意义：1) 在等待锁时降低功耗（减少热设计压力）；2) 减少总线竞争（普通自旋锁的反复 LDXR 会消耗总线带宽，影响其他核的内存访问）。但 WFE 的唤醒延迟（~10-50ns）比普通自旋的即时重试慢，对极短临界区可能不值得。HFT 中更好的方案是避免锁竞争——用 SPSC 无锁队列替代锁。但如果必须用自旋锁且竞争时间较长，WFE 是合理选择。

## 自测题

1. **WFE 自旋锁比普通自旋锁有什么优势？有什么劣势？**

<details>
<summary>答案</summary>

**优势**：
1. 低功耗——等待时 CPU 不 100% 占用
2. 减少总线竞争——不反复 LDXR

**劣势**：
1. 唤醒延迟——WFE 唤醒需要 ~10-50ns，比普通自旋的即时重试慢
2. 可能被误唤醒——中断、调试事件等可能唤醒 WFE，醒来后需重新检查条件
3. 复杂度——需要 unlock 时 SEV 唤醒
</details>

2. **WFE 为什么可能被误唤醒？醒来后应该做什么？**

<details>
<summary>答案</summary>

WFE 等待的是"事件"（Event），但事件不只来自 SEV——**中断、调试异常、系统事件**等都可能产生事件唤醒 WFE。因此 WFE 醒来后**不能假设锁已释放**，必须重新执行 `ldxr` 检查锁状态。代码中 `2: wfe; b 1b` 就是醒来后跳回 `1:` 重新检查。
</details>

3. **unlock 时为什么用 STLR 而不是 STR + SEV？**

<details>
<summary>答案</summary>

`STLR`（Store-Release）自带 release 屏障，保证临界区内的写在锁释放前对其他核可见。如果用 `STR + SEV`，需要额外加 DMB 保证顺序。`STLR` 一步完成 Store + Release 语义，更高效。SEV 仍然需要（唤醒 WFE 等待者），但 STLR 替代了 `STR + DMB`。
</details>

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — LDXR/STXR 基础
- [§20.2 原子操作实现模式](02-atomic-patterns.md) — 普通自旋锁代码
- [Ch18 §18.4 Acquire/Release](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md) — STLR 详解
