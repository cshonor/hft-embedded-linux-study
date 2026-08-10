# 1.2 四个异常等级 EL

> 来源：§1.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ARMv8-A 用 4 个异常等级（EL0-EL3）替代 ARMv7 的 7 种工作模式，这是架构级重大变革。

## 核心要点

权限：**EL0 < EL1 < EL2 < EL3**

| 等级 | 角色 | 对应软件 |
|------|------|----------|
| EL0 | 用户态 | Linux 应用 |
| EL1 | 内核态 | Linux 内核 |
| EL2 | 虚拟化 | Hypervisor |
| EL3 | 安全监控 | Secure Monitor (TrustZone) |

- 发生异常 → **升到更高 EL**（不能异常降级）；返回用 **ERET**
- 普通 Linux：应用 EL0、内核 EL1；EL2/EL3 多由固件（TF-A）处理
- ARMv7 的 7 种模式（User/FIQ/IRQ/SVC/Abort/Undef/System）被 EL 取代

## HFT 关联

HFT 交易系统运行在 EL0（用户态），内核在 EL1。低延迟优化中，理解 EL 边界至关重要：
- 系统调用（SVC）会从 EL0 切到 EL1，有上下文切换开销 → HFT 尽量减少 syscall
- EL2（虚拟化）会引入额外地址翻译（Stage-2 MMU）→ 裸机部署比虚拟机延迟更低
- EL3（TrustZone）可用于安全关键代码隔离，但世界切换开销大，不适合热路径

## 自测题

1. Linux 用户程序和内核分别运行在哪个 EL？
<details><summary>答案</summary>
用户程序 EL0，内核 EL1。
</details>

2. 为什么 ARMv8 抛弃了 ARMv7 的 7 种工作模式？
<details><summary>答案</summary>
7 种模式设计复杂、权限层次不清晰。4 个 EL 提供清晰的权限层级，简化异常处理和特权管理。
</details>

3. 异常能"降级"吗？返回原等级用什么指令？
<details><summary>答案</summary>
异常不能降级——只能升到更高或同级 EL。返回原等级用 ERET 指令，原子恢复 ELR→PC 和 SPSR→PSTATE。
</details>

## 参考与延伸

- 原书 §1.2
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- ARM ARM §D1.3
