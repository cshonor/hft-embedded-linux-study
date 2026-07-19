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

- **调用开销：** 参数传递、栈、分支、阻止内联
- **`static inline`** + 同 TU；或 **模板/宏**（权衡可读性）

**HFT：** 热路径 **禁止虚函数 per tick**（或 PGO/devirtualize）；小函数 `always_inline`。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§5.4 ←](./section-5.4-消除循环的低效率.md) · [本章导读](../README.md) · [§5.6 →](./section-5.6-消除不必要的内存引用.md)
