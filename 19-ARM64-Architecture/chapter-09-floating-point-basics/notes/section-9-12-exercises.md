## §9.12 练习题

> **Ch 9 · 浮点简介** · [章导读](../README.md)

---

### 练习覆盖范围（原书）

| 类型 | 对应节 | 建议 |
|------|--------|------|
| **位域 S/exp/f** | §9.4 | 解码一个 hex float |
| **五类值** | §9.5–9.6 | 分类 exp/frac 模式 |
| **CPACR 初始化** | §9.8 | 口述步骤 |
| **VLDR/VMOV/VCVT** | §9.9–9.11 | 区分三条指令 |
| **±0, NaN** | §9.6 | 比较与传播 |

---

### 自测 Checklist

- [ ] 写出 **单精度** S/exp/f 宽度与 **bias 127**
- [ ] 列举 **Normal / Subnormal / ±0 / ∞ / NaN** 判定条件
- [ ] 解释 **为何先开 CPACR**
- [ ] **`VMOV r,s` vs `VCVT`** 各何时用
- [ ] 口述 **float 动态范围** vs **int32** 量级

---

### 与下一章

→ **[Ch10 舍入与异常](../../chapter-10-floating-point-rounding-exceptions/)** — **跳过**（主线）或 FPU 路径续读
