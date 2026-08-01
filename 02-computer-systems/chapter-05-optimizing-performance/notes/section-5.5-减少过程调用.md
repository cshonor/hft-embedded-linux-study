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

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§5.4 ←](./section-5.4-消除循环的低效率.md) · [本章导读](../README.md) · [§5.6 →](./section-5.6-消除不必要的内存引用.md)
