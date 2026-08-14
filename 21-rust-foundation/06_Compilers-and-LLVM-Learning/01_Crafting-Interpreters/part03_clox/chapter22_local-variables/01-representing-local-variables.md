# 第 22 章 · Local Variables（局部变量） · §22.1 表示局部变量（Representing Local Variables）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-22-3.md](./02-22-3.md)

---

**核心思想**：局部变量 = **栈上固定槽位**，与 ch15 **Value Stack** 合一（非单独数组）。

**`Compiler` 编译期模拟栈**：

```c
typedef struct {
  Token name;
  int depth;      // 声明时的 scopeDepth；-1 表示已就绪可用
  bool isCaptured; // 闭包用（ch24 后续）
} Local;
```

| 字段 | 含义 |
|------|------|
| **`locals[]`** | 当前函数/块内已知局部变量列表 |
| **`localCount`** | 已登记数量 |
| **`scopeDepth`** | 当前块嵌套深度 |
| **slot index** | 通常 = 在 `locals` 中的下标 = 相对栈帧底的偏移 |

**编译表达式/声明时**：维护「栈深度」与 **`locals`** 同步——每声明一个局部，等价于 **预留一个栈槽**。

**对照 jlox [ch8](../../part02_jlox/chapter08_statements-and-state/README.md)**：`Environment` 链 + 按名查找；clox **编译期绑定 slot**。

**对照 jlox [ch11](../../part02_jlox/chapter11_resolving-and-binding/README.md)**：`distance` 静态绑定；clox 局部变量 **直接 emit slot 号**。

---
