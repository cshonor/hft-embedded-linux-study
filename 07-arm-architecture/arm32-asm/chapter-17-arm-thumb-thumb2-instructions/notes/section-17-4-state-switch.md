## §17.4 ARM 与 Thumb 状态切换 — BX 等

> **Ch 17 · ARM、Thumb 和 Thumb-2 指令** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 谁需要切换状态

**ARM7TDMI 等 v4T：** 同芯片可执行 **ARM 32** 与 **Thumb 16/32（Thumb-2 前多为 16）** — 由 **CPSR.T** 区分。

**Cortex-M4：** **不需要** — 本章 §17.4 主要为 **ARM7 + 读 Linux ARM32 启动/异常** 服务。

---

### BX — Branch and Exchange

[Ch8 §8.2](../chapter-08-branches-loops/notes/section-8-2-branches.md) 已引入：

```asm
    BX      r0          ; PC ← r0 & ~1 ；T ← r0[0]
```

| **Rm bit 0** | 跳转后状态 |
|--------------|------------|
| **0** | **ARM** 32-bit |
| **1** | **Thumb** |

**PC 对齐：** 硬件 **忽略 bit0** 作为地址 — bit0 **只存状态**。

---

### 相关指令

| 指令 | 作用 |
|------|------|
| **`BX Rm`** | 跳转 + 交换状态 |
| **`BLX Rm`** | **带链接** 的 BX — LR ← 返回址（bit0 反映 **调用者** 状态） |
| **`BLX imm`（ARM）** | 直接 **BL 到 Thumb 子程序** — 常配合 **Veneer** |

**返回：** 从 Thumb 子程序返 ARM 调用者 → **`BX lr`**，且 **LR 的 bit0 必须为 1**（Thumb 返回址）。

---

### 与异常的关系（ARM7）

[Ch14](../chapter-14-exception-handling-arm7tdmi/)：**异常入口** 硬件 **清 T** → handler 以 **ARM** 运行；返回 **`SUBS pc,lr,#n`** 恢复 **SPSR 含 T 位** → 可回到 **Thumb 应用**。

---

### 符号与链接：Thumb 函数地址

C/汇编 **函数指针** 常 **或上 1** 表示 Thumb：

```c
void (*fp)(void) = (void (*)(void))((uint32_t)ThumbFunc | 1);
```

链接脚本 **`/thumb`** 段 — 符号 **LSB=1** 供 **BX** 识别。

---

### Cortex-M 上的 BX

M4 **始终 Thumb** — **`BX lr`** 仍用于 **异常返回（EXC_RETURN）** 等特殊 LR 值（[Ch15](../chapter-15-exception-handling-v7m/notes/section-15-5-stack-frames.md)），**不是** ARM/Thumb 切换。

---

### 可复述要点

1. **bit0=0 → ARM，bit0=1 → Thumb** — **`BX` 唯一通用切换门**。  
2. **`BL` alone 不切状态** → 需要 **Veneer（§17.5）**。  
3. **M4 可跳过切换细节**，但 **EXC_RETURN 的 BX lr** 仍要懂。
