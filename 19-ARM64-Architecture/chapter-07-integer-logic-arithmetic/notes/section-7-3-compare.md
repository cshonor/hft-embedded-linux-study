## §7.3 比较指令 — CMP · CMN · TST · TEQ

> **Ch 7 · 整数逻辑与算术** · [章导读](../README.md)

---

### 共性：只改标志，不写结果

四条 **比较/测试** 指令执行 ALU 运算，**丢弃结果**，仅更新 **N Z C V**。

---

### CMP — Compare（减）

```asm
        CMP     r0, r1          ; 伪：r0 - r1，只设标志
        CMP     r0, #10
```

| 后续 | 含义 |
|------|------|
| **EQ** | r0 == r1（Z=1） |
| **NE** | 不等 |
| **GT/GE/LT/LE** | **有符号** 比较（看 N,V,Z） |
| **HI/HS/LO/LS** | **无符号** 比较（看 C,Z） |

**Ch3 阶乘：** `CMP r0, #1` + 条件执行 / IT — 本章给出 **CMP 本体**。

---

### CMN — Compare Negative（加）

```asm
        CMN     r0, r1          ; r0 + r1，只设标志
```

用于 **与 -r1 比较** 等等价变换；较少见但对称于 CMP。

---

### TST — Test bits（与）

```asm
        TST     r0, #0xFF       ; r0 AND #mask，只设标志
        BEQ     no_low_byte     ; Z=1 → 低 8 位全 0
```

| 用途 | 说明 |
|------|------|
| **某位是否为 1** | `TST r0, #BIT` → **NE** |
| **掩码后是否全 0** | **EQ** |

**驱动：** 读 MMIO 后 `TST` 状态位 — 再分支（常配合 **Ch16** Load）。

---

### TEQ — Test Equivalence（异或）

```asm
        TEQ     r0, r1          ; r0 EOR r1，只设标志
        BEQ     equal           ; Z=1 → 每一位相同
```

**判断两寄存器是否 bitwise 相等** — 比 `CMP` 更适合「全等」语义（含符号无关的位模式）。

---

### 与 C 对照

| C | 汇编 |
|---|------|
| `if (a == b)` | `CMP a,b` / `TEQ` + `BEQ` |
| `if (a > b)` signed | `CMP` + `BGT` |
| `if (x & MASK)` | `TST` + `BNE` |

---

### 可复述要点

1. **CMP=减、CMN=加、TST=AND、TEQ=EOR** — 都只更新标志。  
2. **TST/TEQ** 是 **位级** 测试利器。  
3. 比较后 **紧跟 B.cond 或 IT**（Ch8）。
