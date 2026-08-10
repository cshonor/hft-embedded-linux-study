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

### 典型应用场景与整机组合

**各 EL 层级运行的典型软件：**

| EL | Non-Secure 世界 | Secure 世界 |
|----|-----------------|------------|
| **EL0** | App / libc / 动态链接器（不能操作硬件） | TA (Trusted Application)：指纹解密、密钥运算、DRM |
| **EL1** | Linux 内核 / RTOS（MMU、调度、中断、驱动） | OP-TEE 安全内核（管理 TA、安全内存、安全中断） |
| **EL2** | KVM Hypervisor / VHE 宿主内核（可选） | 无 S-EL2（ARMv8.4 定义，极少使用） |
| **EL3** | —（不区分世界） | TF-A BL31 Secure Monitor（世界切换、PSCI、FIQ 路由） |

> NS 世界和 S 世界不能直接互相跳转，必须经 EL3 中转。普通 Linux (NS) 完全不能访问 S-EL1 的内存，由硬件隔离。

**4 种典型整机运行组合：**

| 案例 | EL3 | EL2 | EL1 | EL0 | 说明 |
|------|-----|-----|-----|-----|------|
| **手机 Android** | TF-A BL31 | (不用) | NS: Android Linux 内核<br>S: OP-TEE | NS: App<br>S: TA 指纹/密钥 | 有 TrustZone，无虚拟化；Linux 调安全服务走 SMC→EL3→OP-TEE |
| **ARM 服务器 KVM+VHE** | TF-A BL31 | NS: 宿主 Linux 内核 (VHE) | NS: 客户机内核<br>S: OP-TEE | NS: 宿主/客户机用户进程 | VHE 让宿主内核直接跑 EL2，减少陷入开销 |
| **裸机/RTOS** | (不用) | (不用) | NS: 裸机程序/FreeRTOS | (不用) | 无 TrustZone、无虚拟化；直接在 EL1 操作硬件 |
| **客户机调用 TrustZone** | TF-A BL31 | NS: KVM Hypervisor | NS: 客户机内核 | NS: 客户机 App | 客户机 SMC 先被 EL2 拦截，再由 Hypervisor 代理转发到 EL3 |

**客户机 SMC 代理路径（SMC Proxying）：**

虚拟机内部的客户机 OS 不能直接执行 SMC 到 EL3——SMC 会被 EL2 拦截：

```
客户机 NS-EL1 执行 SMC
  → 硬件 trap 到 NS-EL2（Hypervisor 拦截 SMC）
  → Hypervisor 检查请求是否合法（安全策略网关）
  → 合法 → Hypervisor 执行 SMC → trap 到 S-EL3
  → BL31 处理后返回 Hypervisor → Hypervisor 返回客户机
  → 不合法 → Hypervisor 直接拒绝，返回错误码
```

Hypervisor 在这里充当安全网关，可以拒绝、伪造或转发 SMC 请求。Linux KVM-ARM 通过 `KVM_CAP_ARM_SMCCC` 支持此机制。

**PSCI 电源管理：**

所有 ARM64 Linux 内核的 CPU hotplug、suspend、restart 最终都走 SMC → EL3 BL31。内核中的 `psci.c` 驱动封装这些 SMC 调用。EL3 是唯一能操作 CPU 复位/电源硬件的层级，Linux 内核不能直接控制。

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

7. **普通 Linux 内核可以直接访问 OP-TEE (S-EL1) 的内存吗？为什么？**

<details>
<summary>答案</summary>

**不能。** 安全世界和非安全世界由硬件隔离——NS-EL1 的页表中安全世界的物理地址被标记为不可访问，MMU 硬件强制隔离。Linux 要使用安全服务必须通过 SMC 陷入 EL3，由 BL31 中转到 S-EL1 的 OP-TEE，OP-TEE 处理后原路返回。密钥等敏感数据永远不会暴露给 NS 世界。
</details>

8. **客户虚拟机内核执行 SMC 指令，会直接到达 EL3 吗？完整的路径是什么？**

<details>
<summary>答案</summary>

**不会直接到达 EL3。** 完整路径（SMC Proxying）：

1. 客户机 NS-EL1 执行 SMC → 硬件 trap 到 NS-EL2（Hypervisor 拦截）
2. Hypervisor 检查请求是否合法
3. 合法 → Hypervisor 执行 SMC → trap 到 S-EL3（BL31 处理）
4. BL31 返回 Hypervisor → Hypervisor 返回客户机

Hypervisor 充当安全网关，可以拒绝、伪造或转发 SMC 请求。这是虚拟化安全的重要机制——防止恶意客户机直接访问安全世界。
</details>

9. **Linux 内核要重启 CPU（如 `reboot` 系统调用），实际是怎么操作的？为什么不直接复位？**

<details>
<summary>答案</summary>

Linux 内核不能直接操作 CPU 复位硬件——它通过 `psci.c` 驱动执行 SMC 调用，陷入 EL3，由 TF-A BL31 完成底层电源控制（PSCI `SYSTEM_RESET`）。EL3 是唯一能操作 CPU 复位/电源硬件的层级。这种设计保证了安全控制——即使内核被攻破，攻击者也无法绕过 EL3 固件直接控制硬件电源。
