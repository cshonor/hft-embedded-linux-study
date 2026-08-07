## 5.9 提高并行性

> **Ch5 §5.9** · [章导读](../README.md) · 上节 [§5.8 ←](./section-5.8-循环展开.md) · 下节 [§5.10 →](./section-5.10-优化合并代码的结果小结.md)

---

#### 5.9.1 多个累积变量

```c
// 2 路展开 + 双累加器，打破 loop-carried dep
x0 = ident; x1 = ident;
for (i = 0; i < n-1; i += 2) {
    x0 = x0 + data[i];
    x1 = x1 + data[i+1];
}
acc = (x0 + x1) + ...;
```

`x0` 与 `x1` **无依赖** — CPU 可并行执行。

#### 5.9.2 重新结合变换 (Reassociation)

```c
// 结合律对浮点不严格成立！整数 + 在数学上 OK
// (a + data[i]) + data[i+1]  vs  a + (data[i] + data[i+1])
```

- 编译器在 `-ffast-math` 下对浮点重结合 — **HFT 慎用** 于价格累加
- 整数 tick 求和、checksum 可重结合

---

### 常见陷阱

1. **多累加器用太多导致寄存器溢出** — 2-4 路多累加器通常最佳；超过后寄存器不够用，spill 到栈内存，CPE 反弹。用 `perf annotate` 检查是否出现栈访问。
2. **浮点重结合改变结果** — 浮点加法**不严格满足结合律**（`(a+b)+c ≠ a+(b+c)`）。`-ffast-math` 允许重结合但改变舍入。HFT 价格累加**慎用**；整数 tick 求和/checksum 可以。
3. **以为打破依赖就能无限并行** — 受限于功能单元吞吐上限（如每周期最多 2 个 load）。打破依赖只是消除了串行瓶颈，实际并行度还受硬件资源限制。

### 自测题

<details>
<summary>1. 多累加器为什么能提高并行性？</summary>

单累加器 `acc = acc + data[i]` 形成**循环携带依赖**（loop-carried dependency）——每次迭代必须等上次的 `acc`。双累加器 `x0 = x0 + data[i]; x1 = x1 + data[i+1]` 之间**无依赖**，CPU 可并行执行两条独立的加法。
</details>

<details>
<summary>2. 重新结合变换（reassociation）是什么？浮点能用吗？</summary>

改变运算的括号顺序：`(a + data[i]) + data[i+1]` → `a + (data[i] + data[i+1])`。后者打破了 `a` 的循环携带依赖。**整数**可以自由重结合；**浮点**不严格满足结合律，`-ffast-math` 才允许，但会改变舍入结果，HFT 价格累加慎用。
</details>

<details>
<summary>3. 多累加器展开到几路最佳？为什么不是越多越好？</summary>

通常 **2-4 路**最佳。超过后：①寄存器不够用，spill 到栈内存（额外 load/store）；②代码膨胀增加 icache 压力；③受功能单元吞吐上限限制（如每周期最多 2 个 load），更多路也无法并行。用 `perf` 验证实际 CPE。
</details>

---

← [§5.8 ←](./section-5.8-循环展开.md) · [本章导读](../README.md) · [§5.10 →](./section-5.10-优化合并代码的结果小结.md)
