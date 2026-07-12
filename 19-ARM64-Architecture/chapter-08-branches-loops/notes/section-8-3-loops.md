## §8.3 循环 — While · For · Do-While

> **Ch 8 · 分支与循环** · [章导读](../README.md)

---

### While — 先判条件

```c
while (cond) { body; }
```

```asm
        B       exit_when_false    ; 或先 CMP + B{cond} 到 exit
loop
        ; body
        CMP     r0, #limit
        BLT     loop               ; 仍满足则继续
exit_when_false
```

**模式：** **入口比较** → 不满足 **跳过整个循环** → 体 → **尾部比较跳回**。

---

### For — 向下计数（书中推荐）

```c
for (i = n; i != 0; i--) { body; }
```

```asm
        MOV     r1, #n
loop
        ; body
        SUBS    r1, r1, #1        ; 减 1 且更新 Z
        BNE     loop              ; Z=0? 否 → 继续
```

| 为何向下计数 | 说明 |
|--------------|------|
| **`SUBS` + `BNE`** | **一条减法带标志** — 省单独 **`CMP r1,#0`** |
| **代码更小更快** | For 热循环常用 idiom |
| **向上计数** | 需 `ADDS`+`CMP` 上限或 `SUBS` 算剩余 |

**Ch3 阶乘** 即 **递减 + 条件** 变体 — 本章给出 **通用 For 模板**。

---

### Do-While — 至少执行一次

```c
do { body; } while (cond);
```

```asm
loop
        ; body
        CMP     r0, #0
        BNE     loop              ; 条件真则回顶
```

**用途：** 读硬件直到 ready、至少读一次 FIFO 等。

---

### 与 C 编译器

现代编译器对 **已知次数小循环** 也会 **向下计数** 或 **完全展开**（§8.5）。读反汇编时认 **`SUBS; BNE`** 模式。

---

### 可复述要点

1. **While** = 前测 + 体 + 回跳。  
2. **For 汇编优选 `SUBS` 向下 + `BNE`** — 少一条 CMP。  
3. **Do-While** = 体在前、条件在后。
