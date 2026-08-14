# 5. Summary（小结）

> [← 章索引](./README.md)

```text
unsafe 关键字 → 超能力 → UB 责任 → 边界/文档/Miri
```

## 三句话

1. `unsafe fn` 立契约、`unsafe {}` 履行契约 — **显式证明责任**，不是关掉借用检查。  
2. **Validity / 别名 / 布局** 是 UB 三大契约；panic 有定义、**不是** UB。  
3. **最小块 + SAFETY 注释 + Miri** 关住风险。

## 下一章

→ [第 10 章 Concurrency](../Chapter-10-Concurrency-and-Parallelism/README.md)

## 索引

[01](./01-unsafe-keyword.md)–[12](./12-check-your-work.md)
