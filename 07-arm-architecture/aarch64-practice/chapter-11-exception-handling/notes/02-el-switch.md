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

### EL0-EL3 完整层级

数字越大，CPU 特权越高；EL3 最高，EL0 最低。

| 等级 | 角色 | 运行软件 | 说明 |
|------|------|----------|------|
| **EL0** | 用户态（非特权） | Linux 应用程序 | 普通用户进程，不能直接操作硬件/系统寄存器；通过 SVC 陷入 EL1 |
| **EL1** | 内核态（OS 特权） | Linux 内核 | 进程调度、内存管理、驱动；虚拟机客户 OS 内核也运行在此层 |
| **EL2** | 虚拟化层 | Hypervisor（KVM-ARM） | 硬件虚拟化，管理虚拟机；使用 HVC 指令陷入；可选，不开启虚拟化时不启用 |
| **EL3** | 安全监控（最高特权） | Secure Monitor（TF-A BL31, TrustZone） | 安全世界/非安全世界切换；处理 SMC 调用，电源管理 PSCI；始终运行在安全世界 |

### 切换指令

```
SVC  : EL0 → EL1          （普通系统调用）
HVC  : EL0/EL1 → EL2      （Hypervisor 调用；EL0 发 HVC 需 HCR_EL2.TGE=1 等配置，实际场景少）
SMC  : EL0/EL1/EL2 → EL3  （TrustZone 安全监控调用）
ERET : 高 EL → 低 EL      （异常返回，从 ELR/SPSR 原子恢复）
```

### 安全世界与非安全世界

```
     Secure World              Non-Secure World
  ┌──────────────────┐       ┌──────────────────┐
  │ EL3 Secure Monitor│       │ EL2 Hypervisor   │
  │ (TF-A/TrustZone)  │       │ (KVM-ARM/VHE)    │
  ├──────────────────┤       ├──────────────────┤
  │ S-EL1 Trusted OS  │       │ NS-EL1 Kernel    │
  │ (OP-TEE OS)       │       │ (Linux kernel)   │
  ├──────────────────┤       ├──────────────────┤
  │ S-EL0 Trusted App │       │ NS-EL0 User app  │
  │ (密钥/数字版权)    │       │ (普通用户进程)    │
  └──────────────────┘       └──────────────────┘
```

| 规则 | 说明 |
|------|------|
| EL3 永远 Secure | EL3 只存在于安全世界，是安全世界的入口 |
| EL2 固定 Non-Secure | 标准 ARMv8-A 模型下 EL2 在非安全世界（ARMv8.4 引入 S-EL2 但实际产品极少使用） |
| EL0/EL1 可跨世界 | S-EL0/S-EL1 运行 OP-TEE 等安全 OS；NS-EL0/NS-EL1 运行普通 Linux |
| 不是所有 SoC 实现全部 EL | 无虚拟化的芯片可以没有 EL2 |

### VHE 虚拟化扩展（ARMv8.1）

VHE（Virtualization Host Extensions）让宿主 Linux 内核直接运行在 EL2，而不需要在 EL1 运行宿主内核再通过 trap 处理 EL2 事务。客户机内核依然运行在 EL1。

| 对比 | 无 VHE | 有 VHE |
|------|--------|--------|
| 宿主内核运行在 | EL1 | EL2 |
| 宿主访问 EL2 寄存器 | 需 HVC 陷入 | 直接访问 |
| 陷入开销 | 每次需 EL1→EL2 切换 | 无额外陷入 |
| 客户机内核 | EL1 | EL1（不变） |

> VHE 对延迟敏感型负载（如 HFT）有实际意义：减少一层陷入意味着更少的不确定延迟。

### ERET 的原子性

ERET 是**原子操作**：同时恢复 PC（从 ELR）和 PSTATE（从 SPSR），包括 EL 切换。中间不会被中断打断。这保证了异常返回的安全性。

## HFT 关联

HFT 交易系统的 EL 切换开销是延迟优化的重点：

| 场景 | 切换路径 | 开销量级 | HFT 应对策略 |
|------|----------|----------|-------------|
| 系统调用 | EL0→EL1 (SVC) | 1-3μs | busy-polling 替代 read/epoll；共享内存替代 IPC |
| 虚拟化陷入 | EL1→EL2 (HVC) | 2-5μs | VHE 让宿主内核直接跑 EL2，省掉一层陷入 |
| 安全调用 | EL0/1→EL3 (SMC) | 5-10μs+ | HFT 一般不涉及安全世界；若用硬件加密卡需评估 SMC 开销 |
| 中断处理 | EL0→EL1 (IRQ) | 100-500ns | 裸金属方案直接在 EL1 跑交易逻辑，避免 EL 切换 |

**关键点：** ERET 的原子性保证了异常返回延迟的可预测性——ERET 不会被中断打断，从 SPSR 恢复 PSTATE（含 EL）和从 ELR 恢复 PC 是不可分割的。在裸金属 HFT 方案中，可以直接在 EL1 运行交易逻辑，完全避免 EL0→EL1 的系统调用开销。

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

- SVC：EL0 → EL1（系统调用；EL1 也可执行 SVC 但目标是自身，实际场景少）
- HVC：EL0/EL1 → EL2（EL0 发 HVC 需 HCR_EL2.TGE=1 等配置）
- SMC：EL0/EL1/EL2 → EL3（TrustZone 安全监控调用）
- ERET：高 EL → 低 EL（异常返回，原子恢复 PC + PSTATE）

> 复习：[§11.2 EL 完整层级表](#el0-el3-完整层级) · [§11.2 安全世界](#安全世界与非安全世界)
</details>

## 参考与延伸

- [§11.1 异常类型](01-exception-types.md) — 哪些事件会触发异常
- [§11.3 异常向量表](03-vector-table.md) — 异常入口地址怎么找
- [§11.6 EL2→EL1 实验](06-el2-to-el1.md) — 启动时降级到 EL1 的实现

### 补充自测题

4. **EL3 可以运行在非安全世界吗？EL2 可以运行在安全世界吗？**

<details>
<summary>答案</summary>

**EL3 永远在安全世界**——它是安全世界的入口，负责安全/非安全世界切换。**EL2 在标准 ARMv8-A 模型下固定在非安全世界**。ARMv8.4 引入了 S-EL2（Secure EL2），允许 EL2 运行在安全世界，但实际产品极少使用。
</details>

5. **VHE 解决了什么问题？对 HFT 有什么意义？**

<details>
<summary>答案</summary>

VHE（ARMv8.1）让宿主 Linux 内核直接运行在 EL2，而不需要通过 HVC 陷入来访问 EL2 寄存器。**对 HFT 的意义**：如果 HFT 系统跑在虚拟化环境，VHE 消除了宿主内核 EL1→EL2 的陷入开销（2-5μs/次），减少不确定延迟。客户机内核仍在 EL1，不受影响。
</details>

6. **一个没有虚拟化的嵌入式 SoC，从 EL3 启动后可以直接降到 EL1 吗？中间需要经过 EL2 吗？**

<details>
<summary>答案</summary>

**可以直接从 EL3 降到 EL1**，不需要经过 EL2。ERET 可以从任何高 EL 返回到任何低 EL（通过设置 SPSR_EL3.M[3:0]）。但如果该 SoC 实现了 EL2，启动代码通常先 EL3→EL2（初始化 Hypervisor 配置），再 EL2→EL1。无 EL2 的 SoC 直接 EL3→EL1 即可。参见 [§11.6 EL2→EL1 实验](06-el2-to-el1.md)。
</details>
