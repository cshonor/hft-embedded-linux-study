## §7.6 位操作指令 — BFI · UBFX · RBIT 等

> **Ch 7 · 整数逻辑与算术** · [章导读](../README.md)

---

### 为何需要专用位域指令

协议寄存器、外设 **CR 寄存器** 常为 **bitfield** — 用 AND/ORR 掩码可以，但 **BFI/UBFX** 一条完成 **提取/插入/清除**，少指令、少临时寄存器。

**Cortex-M3/M4+** 引入（ARM7 可能无 — 查具体核）。

---

### 常用指令

| 指令 | 作用 |
|------|------|
| **`BFI Rd, Rn, #lsb, #width`** | **Bit Field Insert** — 把 Rn 低 width 位插入 Rd 的 lsb 起 |
| **`BFC Rd, #lsb, #width`** | **Bit Field Clear** — Rd 中该域清 0 |
| **`UBFX Rd, Rn, #lsb, #width`** | **Unsigned** 提取 width 位 → Rd |
| **`SBFX Rd, Rn, #lsb, #width`** | **Signed** 提取并符号扩展 |
| **`RBIT Rd, Rn`** | **Reverse BIT order** — 32 bit 位序完全反转 |

**示例（概念）：**

```asm
        UBFX    r1, r0, #8, #4      ; 取 r0 的 bit[11:8] 无符号到 r1
        BFI     r2, r1, #0, #4      ; 插入 r2 低 4 位
```

---

### 与驱动 / 设备树

| 场景 | 用法 |
|------|------|
| **读 STATUS 某 3 bit 域** | `UBFX` 代替 `LSR`+`AND` |
| **写 CONTROL 某域不改其他位** | `BFC` + `BFI` 或 read-modify-write |
| **协议 bit 序反转** | `RBIT`（如 SPI 某些 LSB-first 硬件） |

**Linux 内核：** C 里 `FIELD_GET`/`FIELD_PREP`（宏）— 汇编层即 **UBFX/BFI** 思想。

**Ch16 MMIO：** Load 字 → **UBFX** 解析 → 分支/返回。

---

### 可复述要点

1. **UBFX/SBFX=提取**，**BFI/BFC=插入/清除** — 寄存器 bitfield 专用。  
2. **RBIT** 整字位反转 —  niche 但省循环。  
3. 读 **datasheet 寄存器图** 时直接映射到这些指令。
