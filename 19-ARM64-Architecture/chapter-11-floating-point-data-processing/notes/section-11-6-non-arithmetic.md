## §11.6 非算术指令 — VABS · VNEG

> **Ch 11 · 浮点数据处理** · [章导读](../README.md)

---

### VABS — 绝对值

```asm
        VABS.F32    s0, s1          ; s0 = |s1| — 强制 **符号位 = 0**
```

**符号-幅度格式：** 只改 **S 位** — 尾数/指数不变。

---

### VNEG — 求反

```asm
        VNEG.F32    s0, s1          ; s0 = -s1 — **翻转符号位**
```

---

### 与 NaN / 异常

| 点 | 说明 |
|----|------|
| **sNaN** | VABS/VNEG **通常不触发 IOC** — 仅改符号位，不“运算” payload |
| **qNaN** | 符号可能变 — 仍 NaN |
| **±0** | ABS 得 **+0**；NEG 翻转 ±0 |

**对比 VADD 等：** 真算术更易 **Invalid** — ABS/NEG 是 **位级** 操作。

---

### C 对照

`fabsf` / 一元 `-` — 编译器出 VABS/VNEG 或等价。

---

### 可复述要点

1. **VABS/VNEG 只动符号位**。  
2. **一般不触发 Invalid** — 即使 sNaN。  
3. 需要 **数学 sqrt/round** 用 **VSQRT** 等 — 非本章两条。
