## 5.5 减少过程调用

> **Ch5 §5.5** · [章导读](../README.md) · 上节 [§5.4 ←](./section-5.4-消除循环的低效率.md) · 下节 [§5.6 →](./section-5.6-消除不必要的内存引用.md)

---

```c
// 慢：每元素一次 call
for (i = 0; i < n; i++)
    acc = combine(acc, data[i]);

// 快：宏/内联/手写操作符在循环内
for (i = 0; i < n; i++)
    acc = acc + data[i];
```

- **调用开销：** 参数传递、栈、`call`/`ret`、打乱流水线  
- **`inline` 是 C/C++ 关键字**（不是汇编指令）— 请编译器把函数体嵌到调用处；成功则机器码里常 **看不到 `call`**  
- **`static inline`** + 同 TU；或 **模板/宏**（权衡可读性）

**HFT：** 热路径 **禁止虚函数 per tick**（或 PGO/devirtualize）；价差/小工具函数 `inline`/`always_inline`，少栈往返、寄存器更连着用。机制细节 → [Ch3 §3.7.7](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md)。

---

### 常见陷阱

1. **热循环里调虚函数** — 虚函数通过 vtable 间接调用，编译器无法内联，每次 `call` + vtable 查找。HFT 热路径用 PGO/devirtualize 或直接 `final` + 非虚接口。
2. **以为 `inline` 关键字保证内联** — `inline` 只是**建议**，编译器可能拒绝（函数太大、递归、跨 TU 无 LTO）。用 `__attribute__((always_inline))` 或 `static inline` + 同 TU 强制。
3. **忽略 `call`/`ret` 对流水线的影响** — 每次函数调用有 `call`（PC 跳转→控制冒险）+ 栈帧操作 + `ret`（PC 恢复→控制冒险）。热循环内频繁调用 = 频繁冲刷。

### 自测题

<details>
<summary>1. 循环内函数调用有哪些开销？</summary>

①参数传递（压栈或寄存器）；②`call` 指令（PC 跳转→控制冒险）；③栈帧建立/销毁；④`ret` 指令（PC 恢复→控制冒险）；⑤打乱流水线（分支预测）。内联后这些开销全部消失，机器码里看不到 `call`。
</details>

<details>
<summary>2. inline 关键字能保证内联吗？什么情况不会？</summary>

**不能。** `inline` 只是建议。编译器可能拒绝：函数太大、递归、跨编译单元无 LTO。用 `static inline` + 同 TU，或 `__attribute__((always_inline))` 强制。但过度 `always_inline` 会增加代码膨胀和 icache 压力。
</details>

<details>
<summary>3. HFT 热路径中虚函数为什么是性能杀手？</summary>

虚函数通过 **vtable** 间接调用：①编译器无法内联（不知道运行时调哪个实现）；②每次调用多一次 vtable 内存读取（可能 cache miss）；③`call` 通过寄存器间接跳转，分支预测器更难预测。解法：PGO/devirtualize、`final` 关键字、或编译期多态（模板/C++)。
</details>

---

← [§5.4 ←](./section-5.4-消除循环的低效率.md) · [本章导读](../README.md) · [§5.6 →](./section-5.6-消除不必要的内存引用.md)
