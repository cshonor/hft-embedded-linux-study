# 1.2 四个异常等级 EL

> 来源：§1.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ARMv8-A 用 4 个异常等级（Exception Level, EL0-EL3）替代 ARMv7 的 7 种工作模式，这是架构级重大变革。本节详解每个 EL 的角色、EL 切换规则、以及与 ARMv7 模式的对应关系。

## EL 权限层级

```
权限递增 ↑

  ┌─────────────────┐
  │  EL3            │  Secure Monitor (TrustZone)
  │  安全世界切换     │  ← 最高权限，能访问安全/非安全两个世界
  ├─────────────────┤
  │  EL2            │  Hypervisor (虚拟化)
  │  虚拟机管理       │  ← Stage-2 MMU 地址翻译
  ├─────────────────┤
  │  EL1            │  Linux Kernel (内核态)
  │  操作系统         │  ← 通常 kernel 跑在这里
  ├─────────────────┤
  │  EL0            │  User Application (用户态)
  │  用户程序         │  ← 最低权限，不能访问硬件
  └─────────────────┘
```

**权限规则**：EL0 < EL1 < EL2 < EL3。高 EL 可以访问低 EL 的资源，反之不行。

## 各 EL 详解

| 等级 | 角色 | 运行的软件 | 权限 | 典型场景 |
|------|------|----------|------|---------|
| EL0 | 用户态 | 应用程序 | 最低 | Linux 用户程序、裸机用户态 |
| EL1 | 内核态 | OS 内核 | 中 | Linux kernel、RTOS |
| EL2 | 虚拟化 | Hypervisor | 高 | KVM、Xen、TF-A |
| EL3 | 安全监控 | Secure Monitor | 最高 | TrustZone 切换、EL3 firmware |

### EL0 — 用户态

- **权限**：不能直接访问硬件（MMIO/SPI/系统寄存器）
- **内存**：只能访问 TTBR0 映射的用户空间（VA bit[63:48]=0）
- **异常**：SVC 系统调用 → 升到 EL1
- **安全**：即使程序被攻破也不影响系统

### EL1 — 内核态

- **权限**：可访问所有系统寄存器、配置 MMU/中断/cache
- **内存**：可访问 TTBR0（用户）和 TTBR1（内核，VA bit[63:48]=1）
- **异常**：IRQ/FIQ/SError/Sync → 在 EL1 处理
- **Linux**：大部分内核代码运行在 EL1

### EL2 — 虚拟化

- **权限**：可拦截 EL1 的 MMU/中断/异常 → 实现虚拟化
- **关键**：Stage-2 MMU（IPA → PA 二次翻译）、虚拟中断注入
- **Linux KVM**：KVM 在 EL2 运行（ARMv8 上 KVM 是 Type-1 半虚拟化）

### EL3 — 安全监控

- **权限**：最高，可以访问安全世界和非安全世界
- **TrustZone**：安全世界有自己的 TTBR、中断、外设
- **切换**：SMC 指令从 EL1 → EL3，EL3 决定路由到安全世界还是返回
- **固件**：通常由 TF-A（Trusted Firmware-A）提供

## EL 切换规则

| 切换方式 | 方向 | 触发 | 返回指令 |
|---------|------|------|---------|
| 异常进入 | 低→高 EL | IRQ/FIQ/SVC/SMC/HVC/SError | - |
| 异常返回 | 高→低 EL | - | ERET |
| 不能直接降级 | - | 没有"异常降级"机制 | - |

### 异常进入流程

```
EL0 执行 SVC #0
    → 硬件自动：
      1. 保存 PSTATE → SPSR_EL1
      2. 保存返回地址 → ELR_EL1
      3. 设置 PSTATE（屏蔽 DAIF 等）
      4. 从 VBAR_EL1 + 偏移取向量
      5. 切换到 EL1（SP_EL1 栈）
    → 跳转到异常向量处理代码
```

### ERET 异常返回

```
ERET
    → 硬件原子操作：
      1. SPSR_EL1 → PSTATE（恢复标志位、DAIF 等）
      2. ELR_EL1 → PC（跳回异常前的地址）
      3. 切换回 EL0
    → 继续执行用户代码
```

## ARMv7 模式 → ARMv8 EL 映射

| ARMv7 模式 | ARMv8 EL | 说明 |
|-----------|---------|------|
| User | EL0 | 用户态 |
| SVC | EL1 | 内核态（系统调用） |
| IRQ | EL1 | 中断处理 |
| FIQ | EL1 | 快速中断 |
| Abort | EL1 | 内存异常 |
| Undef | EL1 | 未定义指令 |
| System | EL1 | 特权模式 |
| Hyp | EL2 | 虚拟化 |
| Monitor | EL3 | 安全监控 |

**ARMv7 的 7 种模式被 4 个 EL 取代**，简化了异常处理和权限管理。

## EL 与栈的关系

| EL | SP 选择 | 说明 |
|----|--------|------|
| EL0 | SP_EL0 | 用户态用 SP_EL0 |
| EL1 | SP_EL1 或 SP_EL0 | 可选（异常进入时通常用 SP_EL1） |
| EL2 | SP_EL2 | Hypervisor 栈 |
| EL3 | SP_EL3 | Secure Monitor 栈 |

每个 EL 有独立的 SP（SP_EL0/1/2/3），异常进入时硬件不自动切换 SP（需软件在 VBAR 中处理）。

## CurrentEL 寄存器

```asm
mrs x0, CurrentEL
lsr x0, x0, #2    ; bit[3:2] = EL number
; x0 = 0 (EL0), 1 (EL1), 2 (EL2), 3 (EL3)
```

## HFT 关联

HFT 交易系统运行在 EL0（用户态），内核在 EL1。低延迟优化中，理解 EL 边界至关重要：
- **系统调用（SVC）会从 EL0 切到 EL1**，有上下文切换开销 → HFT 尽量减少 syscall
- **EL2（虚拟化）会引入额外地址翻译**（Stage-2 MMU）→ 裸机部署比虚拟机延迟更低
- **EL3（TrustZone）可用于安全关键代码隔离**，但世界切换开销大，不适合热路径
- **中断在 EL1 处理** → IRQ 到达后先经过 EL1 陷阱入口，再到用户态通知 → 延迟

## 自测题

1. Linux 用户程序和内核分别运行在哪个 EL？
<details><summary>答案</summary>
用户程序 EL0，内核 EL1。EL2/EL3 由固件（如 TF-A/KVM）管理，Linux 内核通常不直接接触。用户态通过 SVC 指令系统调用进入 EL1。
</details>

2. 为什么 ARMv8 抛弃了 ARMv7 的 7 种工作模式？
<details><summary>答案</summary>
（1）7 种模式设计复杂、权限层次不清晰（IRQ/FIQ/Abort 都是特权但级别相同）（2）4 个 EL 提供清晰的线性权限层级（3）简化异常处理：所有异常统一用 ELR/SPSR 机制（4）更好支持虚拟化（EL2）和安全（EL3）。
</details>

3. 异常能"降级"吗？返回原等级用什么指令？
<details><summary>答案</summary>
异常不能降级——发生异常时只能升到更高或同级 EL。返回原等级用 ERET 指令，它原子地恢复 SPSR→PSTATE 和 ELR→PC，切回异常前的 EL。ERET 是唯一能从高 EL 回到低 EL 的机制。
</details>

4. EL2 的 Stage-2 MMU 对性能有什么影响？
<details><summary>答案</summary>
EL2 引入 Stage-2 MMU（IPA→PA 二次翻译），虚拟机中的地址要经过 Stage-1（Guest VA→Guest IPA）+ Stage-2（IPA→Host PA）两次翻译。即使有硬件页表遍历（PTW），也有额外开销。裸机部署（不用虚拟化）省去 Stage-2 → 延迟更低，HFT 优先选裸机。
</details>

5. 如何在汇编代码中判断当前运行在哪个 EL？
<details><summary>答案</summary>
读 CurrentEL 寄存器：`mrs x0, CurrentEL`，然后右移 2 位：`lsr x0, x0, #2`。结果 0=EL0, 1=EL1, 2=EL2, 3=EL3。Linux 内核启动时 head.S 用此判断是否需要从 EL2/EL3 降级到 EL1。
</details>

## 参考与延伸

- 原书 §1.2
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [1.3 寄存器](03-registers.md)
- ARM ARM §D1.3
