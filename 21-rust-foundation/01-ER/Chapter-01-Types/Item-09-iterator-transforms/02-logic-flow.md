# Item 9 · 逻辑脉络

← [Item 9 目录](./README.md)

```text
while → 下标 for → for-each → 迭代器链（源 + 适配器 + 消费者）
```

### 三段论

1. **源**：`values.iter()` 等  
2. **适配器链**：`filter` → `take` → `map`（**惰性**，直到消费者才跑）  
3. **消费者**：`sum()` / `collect()`  

### 性能与安全

- 下标 `values[i]` → 运行时**边界检查**。
- 迭代器遍历在 LLVM 优化下常能**消除**边界检查 → 有时比手写下标循环更快（见 Book 13.4，仍应 **bench** 验证）。

---
