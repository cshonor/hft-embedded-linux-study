# 4. Summary（小结）

> [← 章索引](./README.md)

第 2 章 **Types** 把内存表示与 Trait 系统连成一条线：

```text
Types in Memory（对齐 → 布局 → 复合类型 → DST/宽指针）
        ↓
Traits and Trait Bounds（分发 → 泛型 trait → 孤儿规则 → 限定 → marker）
        ↓
Existential Types（impl Trait）
```

## 四句话带走

1. **对齐与 `repr`** 决定 FFI 与 UB 边界。  
2. **DST + 宽指针** 是理解 `str`、slice、`dyn Trait` 的钥匙。  
3. **单态化 vs vtable** 是性能与体积的核心权衡；**HFT 热路径优先静态 / enum**。  
4. **`impl Trait`** 隐藏具体类型，仍走静态分发。

## 下一章

→ [第 3 章 Designing Interfaces](../Chapter-03-Designing-Interfaces/3-接口设计-Designing-Interfaces-深度解析.md)：API 设计、对象安全、文档与约束。

## 本章笔记索引

| # | 文件 |
|---|------|
| 01–04 | Types in Memory → [01](./01-alignment.md) … [03](./03-complex-types.md) · [04 hub](./04-dst-wide-pointers.md)（[04.1](./04-1-dst-basics.md)～[04.4](./04-4-containers-ffi-hft.md)） |
| 05–09 | Traits and Trait Bounds → [05 hub](./05-compilation-dispatch.md)（[05.1](./05-1-static-vs-dynamic.md)～[05.4](./05-4-selection-hft.md)）· [06 hub](./06-generic-traits.md)（[06.1](./06-1-associated-vs-generic.md)～[06.2](./06-2-existential-hft.md)）· [07 hub](./07-coherence-orphan-rule.md)（[07.1](./07-1-orphan-rule.md)～[07.3](./07-3-newtype-practice.md)）· [08 hub](./08-trait-bounds.md)（[08.1](./08-1-syntax-static-dynamic.md)～[08.3](./08-3-examples-pitfalls.md)）· [09 hub](./09-marker-traits.md)（[09.1](./09-1-marker-basics.md)～[09.4](./09-4-unsafe-impl.md)） |
| 10 | [10 hub](./10-existential-types.md)（[10.1](./10-1-logic-positions.md)～[10.3](./10-3-limits-selection.md)） |
| 11 | 本文件 |
