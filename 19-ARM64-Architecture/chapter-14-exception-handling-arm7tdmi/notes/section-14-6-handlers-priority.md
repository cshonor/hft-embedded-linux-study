## §14.6 处理程序与优先级

> **Ch 14 · 异常处理：ARM7TDMI** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 异常处理程序 (Handler) 职责

硬件完成 [§14.4](./section-14-4-exception-sequence.md) 后，**软件** 接管：

```
1. 保存会被破坏的 r0–r12（及必要时 lr）到栈
2. 识别/清除中断源（读 VIC、写外设 ACK）
3. 执行服务逻辑
4. 恢复寄存器
5. SUBS pc, lr, #n  →  原子恢复 CPSR + 返回
```

**原则：** handler **短小** — 耗时工作留 **底半部**（Linux **tasklet/workqueue** 思想，[21 驱动](../../21-Linux-Device-Driver/)）。

---

### 返回的原子操作

**目标：** 同时恢复

- **程序计数器** → 被中断的下一条指令  
- **CPSR**（模式、Thumb、I/F 屏蔽、NZCV）→ 进入异常前状态  

**典型尾声：**

```asm
    LDMIA   sp!, {r0-r12, lr}
    SUBS    pc, lr, #4      ; IRQ：#4；Data Abort：#8
```

**`SUBS` 写 PC** 在特权异常模式下 **附带 SPSR → CPSR**。

---

### 嵌套中断（概念）

硬件入口 **自动 I=1（禁 IRQ）** — 默认 **IRQ 不嵌套**。若允许嵌套：

- 在 handler 内 **清外设后** 临时 **开 IRQ**  
- 须保证 **栈深度** 与 **重入安全**  

**FIQ** 可在 IRQ handler 运行时仍响应（若 F 未屏蔽）— 设计 **极紧急** 路径。

---

### 异常优先级（固定硬件顺序）

**从高到低：**

| 优先级 | 异常 |
|--------|------|
| **1（最高）** | **Reset** |
| 2 | **Data Abort** |
| 3 | **FIQ** |
| 4 | **IRQ** |
| 5 | **Prefetch Abort** |
| **6（最低）** | **SVC / Undefined**（同级类，具体以实现为准） |

**口述场景：** 数据中止与 IRQ 同时 → **先处理 Data Abort**；处理完再响应 IRQ。

---

### Handler  vs  Ch13 子程序

| | **BL 子程序** | **异常 handler** |
|---|---------------|------------------|
| 入口 | 软件安排 | **向量强制** |
| LR | 可调 | **硬件赋值**（需 #offset） |
| 状态 | 手动保存 CPSR？ | **SPSR 硬件保存** |
| 返回 | `BX lr` / `LDM … pc` | **`SUBS pc, lr, #n`** |

---

### 可复述要点

1. Handler = **压栈 → 服务 → SUBS 返回**。  
2. 优先级：**Reset > Data Abort > FIQ > IRQ > Prefetch > SVC/Und**。  
3. **短 ISR** — 与 Linux **top half** 同原则。
