## §9.9 浮点数据传输 — VLDR · VSTR · VMOV

> **Ch 9 · 浮点简介** · [章导读](../README.md)

---

### 内存 ↔ FPU：`VLDR` / `VSTR`

```asm
        VLDR    s5, [r6, #8]        ; s5 ← Mem32[r6+8]
        VSTR    s0, [sp, #-4]!      ; 压栈单精度（配合 Ch13）
        VLDR.F32 s1, =3.14159       ; 伪指令 → 文字池（Ch6）
```

| 指令 | 方向 |
|------|------|
| **`VLDR`** | 内存 → **s** |
| **`VSTR`** | **s** → 内存 |

**对齐：** 单精度通常 **4 字节对齐** — 违例 fault（同 Ch2/Ch5）。

**与 Ch5：** 同一 **LDR/STR 寻址模式** — `[Rn, #off]`、`!`、post-index 可用于 **VSTR/VLDR**（语法随汇编器）。

---

### 整数 ↔ FPU：`VMOV`（位拷贝）

```asm
        VMOV    s0, r0              ; 32 bit 模式复制
        VMOV    r0, s0
        VMOV.F32 s1, #1.0            ; 部分「简单」浮点立即数
        VMOV    s2, s0              ; FPU 寄存器间
```

| 重要 | 说明 |
|------|------|
| **r ↔ s VMOV** | **reinterpret** — **不** 做 int→float 数值转换 |
| **真转换** | **`VCVT`**（§9.11） |
| **Ch3 §3.7** | 已演示 r/s 搬运 |

---

### 数据流图

```
Mem  ←VSTR/VLDR→  s0–s31  ←VMOV→  r0–r12
                      ↓
                   VCVT / VADD…（Ch11）
```

---

### 可复述要点

1. **VLDR/VSTR** = 浮点版 Load/Store。  
2. **`LDR =` 浮点常数** 同样走文字池。  
3. **VMOV r,s ≠ VCVT** — 别用 VMOV 当 `(float)int`。
