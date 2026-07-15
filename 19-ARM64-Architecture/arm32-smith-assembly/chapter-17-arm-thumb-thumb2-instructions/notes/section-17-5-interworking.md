## §17.5 如何为 Thumb 编译代码 — Interworking

> **Ch 17 · ARM、Thumb 和 Thumb-2 指令** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 混合 ARM/Thumb 的工程现实

大型项目常见：

| 组件 | 编译模式 |
|------|----------|
| **启动/异常/内核** | **ARM**（历史）或 **Thumb-2**（新） |
| **应用/库** | **`-mthumb`** Thumb 或 Thumb-2 |
| **ROM 库** | 供应商 **ARM** 目标 |

**C 调用 `foo()`** 时，若 **调用者 ARM、被调 Thumb**（或反之）— 直接 **`BL foo`** **不够**。

---

### 问题：`BL` 不改变状态

| 指令 | 跳转 | 切 ARM/Thumb |
|------|------|--------------|
| **`BL label`** | ✓ | **✗** |
| **`BLX` / Veneer 内 **`BX`** | ✓ | **✓** |

**错误场景：** ARM 态 **`BL`** 进 Thumb **`foo`**（地址 bit0=1）— 若 CPU 仍 **ARM 态** 译码 → **未定义指令 / 乱码执行**。

---

### Interworking 与 Veneer（垫片）

**链接器** 在 **跨状态调用** 时自动插入 **Veneer**（约 **8–12 字节**）：

```
ARM 调用者:
    BL      __foo_from_arm_veneer    ; 仍 ARM 态 BL
__foo_from_arm_veneer:
    LDR     r12, =ThumbFoo+1
    BX      r12                      ; 切 Thumb，跳 foo
```

**反向**（Thumb 调 ARM）类似：**LDR + BX**（bit0=0）。

**程序员：** 源码只写 **`foo()`**；**map 文件** 可见 **`veneer`** 符号。

---

### 编译/链接选项（概念）

| 选项 | 作用 |
|------|------|
| **`-mthumb`** | 生成 **Thumb/Thumb-2** 代码 |
| **`-marm`** | 生成 **ARM 32**（ARM7/AArch32） |
| **`-minterwork`** | 允许 **ARM↔Thumb 互调** — 生成 **Veneer 需求** 的 reloc |
| **`-mno-interwork`** | 假定 **单一状态** — M4 常用 |

→ [Ch18 §18.4](../chapter-18-mixing-c-and-assembly/notes/section-18-4-c-asm-calls.md) **C/Asm 互调**

---

### Cortex-M4：无 Veneer

```
永远 Thumb-2 → 无 ARM 态
-mthumb 默认
不需要 interwork 垫片（同状态 BL 即可）
```

**口述：** M4 工程 **看不到 Veneer** 是正常的；读 **ARM9/ARM11 内核或旧 BSP** 仍会遇。

---

### 与密度权衡

全 Thumb **减 ROM**；热点 **ARM**（老项目）— **Interworking 成本** = 每次跨态 **+ veneer 跳转**。Thumb-2 出现后，**全 Thumb-2** 往往是 **更优解** — 密度 + 无 **双态切换**。

---

### 可复述要点

1. **Interworking = 链接器用 Veneer + BX 补 BL 的不足**。  
2. **Veneer 8–12 B** — 跨 ARM/Thumb 每次调用 **小额税**。  
3. **M4 纯 Thumb-2** — **本章 §17.5 作背景**，非日常实操。
