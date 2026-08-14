# 6. Summary（小结）

> [← 章索引](./README.md)

```text
麻烦（正确/性能）→ 模型 → async×并行 → 原子 → 工程实践
```

## 三句话

1. 编译通过 ≠ 并发正确。  
2. **共享内存 / 池 / Actor** + **async vs 并行** 先选型。  
3. **atomics + ordering** 谨慎用；**Loom / TSan / 压测** 守底线。

## 下一章

→ [第 11 章 FFI](../Chapter-11-Foreign-Function-Interfaces/11-外部函数接口-FFI-深度解析.md)

## 索引

[01](./01-correctness.md)–[14](./14-concurrency-testing-tools.md)
