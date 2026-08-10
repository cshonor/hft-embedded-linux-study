# §11.2 异常等级切换

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

异常总是在异常等级（EL）之间切换：SVC 从 EL0 升到 EL1，IRQ 从 EL0/EL1 进入 EL1，HVC 进入 EL2，SMC 进入 EL3。返回用 ERET 原子恢复 PC 和 PSTATE。

## 核心要点

```
EL0 (用户态) --SVC--> EL1 (内核态)
EL0/EL1 --IRQ--> EL1 (内核态中断处理)
EL1 --HVC--> EL2 (Hypervisor)
EL2/EL1 --SMC--> EL3 (Secure Monitor)
```

### 核心规则

| 规则 | 说明 |
|------|------|
| 异常只能**升到更高或同等级** | 不能异常降级（EL1 不能异常到 EL0） |
| `ERET` 返回 | 硬件恢复 ELR→PC、SPSR→PSTATE，切回原等级 |
| 每个异常目标 EL 有独立 SP | SP_EL0/SP_EL1/SP_EL2/SP_EL3 |
| 异常入口硬件自动关中断 | PSTATE.{D,A,I,F} 被设为 1 |

### ERET 的原子性

ERET 是**原子操作**：同时恢复 PC（从 ELR）和 PSTATE（从 SPSR），包括 EL 切换。中间不会被中断打断。这保证了异常返回的安全性。

## HFT 关联

HFT 交易系统通常运行在 EL1（内核态）或 EL0（用户态）。每次系统调用（SVC）都涉及 EL0→EL1 切换，有约 100-200ns 的开销。理解 ERET 的原子性有助于分析中断延迟的确定性——ERET 不会被打断，返回延迟是可预测的。在裸金属 HFT 方案中，可以直接在 EL1 运行交易逻辑，避免 EL 切换开销。

## 自测题

1. **异常可以从 EL1 触发到 EL0 吗？为什么？**

<details>
<summary>答案</summary>

**不可以**。异常只能升到更高或同等级。EL0 是最低等级，无法通过异常进入。从 EL1 回到 EL0 只能通过 `ERET`（异常返回），不是触发异常。
</details>

2. **ERET 指令做了哪些操作？为什么说是原子的？**

<details>
<summary>答案</summary>

ERET 同时恢复：**ELR_ELx → PC**（返回地址）和 **SPSR_ELx → PSTATE**（处理器状态，包括 EL）。说它是原子的因为这两个操作不可分割——中间不会被中断打断，保证了 EL 切换的一致性。
</details>

3. **SVC、HVC、SMC 分别从哪个 EL 触发，到哪个 EL？**

<details>
<summary>答案</summary>

- SVC：EL0 或 EL1 → EL1（系统调用）
- HVC：EL1 → EL2（Hypervisor 调用）
- SMC：EL1 或 EL2 → EL3（安全监控调用）
</details>

## 参考与延伸

- [§11.1 异常类型](01-exception-types.md) — 哪些事件会触发异常
- [§11.3 异常向量表](03-vector-table.md) — 异常入口地址怎么找
- [§11.6 EL2→EL1 实验](06-el2-to-el1.md) — 启动时降级到 EL1 的实现
