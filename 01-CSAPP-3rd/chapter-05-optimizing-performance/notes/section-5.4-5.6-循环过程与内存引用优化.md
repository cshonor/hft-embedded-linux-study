## 5.4–5.6 循环、过程调用与内存引用

### 5.4 消除循环的低效率

**典型杀手：循环内重复计算**

```c
// 慢：每次迭代 strlen
for (i = 0; i < strlen(s); i++) ...

// 快：不变量外提
int len = strlen(s);
for (i = 0; i < len; i++) ...
```

- **归纳变量优化** — 编译器常做，但依赖 `-O` 与可见性
- HFT：循环边界、buffer 长度 **在循环外算好**；tick 处理别每包 `vector.size()` 若可缓存

### 5.5 减少过程调用

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

### 5.6 消除不必要的内存引用

```c
// 慢：每次读写 acc 可能在栈/内存
for (i = 0; i < n; i++)
    acc = combine(acc, data[i]);  // acc 地址可能被 spill

// 快：局部累加器在寄存器
val = ident;
for (i = 0; i < n; i++)
    val = val + data[i];
acc = val;
```

- 编译器需证明 **`acc` 无别名、无逃逸** 才能把 `acc` 留寄存器
- **指针 `&acc` 传出去** → 优化夭折

**HFT：** 热循环用 **局部变量累积**，最后写回；order book 更新批量化，少反复 load/store 同一价位节点。

→ 汇编对照：[Ch 3](../../chapter-03-machine-level-programs/)

---

← [本章导读](../README.md)
