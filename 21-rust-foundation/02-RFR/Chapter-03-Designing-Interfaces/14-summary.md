# 5. Summary（小结）

> [← 章索引](./README.md)

第 3 章 **Designing Interfaces** 用四条准则设计工业级 API：

```text
Unsurprising  → 命名 · 通用 trait · 人体工程学 impl · 包装类型
Flexible      → 泛型 · 对象安全 · 借用/拥有 · 显式 close
Obvious       → 文档 · 类型引导（enum / typestate / must_use）
Constrained   → 可见性 · sealed · 隐藏契约
```

## 组合使用

四条准则**相互牵制**：过度 Flexible 会损害 Obvious；过度 Constrained 会损害 Flexible。在公开 crate 上迭代时，优先 **Obvious + Constrained** 保演进，**Unsurprising** 降学习成本。

## 下一章

→ [第 4 章 Error Handling](../Chapter-04-Error-Handling/4-错误处理-Error-Handling-深度解析.md)

## 本章笔记索引

| 主节 | 文件 |
|------|------|
| Unsurprising | [01](./01-naming-practices.md)–[04](./04-wrapper-types.md) |
| Flexible | [05](./05-generic-arguments.md)–[08](./08-fallible-blocking-destructors.md) |
| Obvious | [09](./09-documentation.md) · [10](./10-type-system-guidance.md) |
| Constrained | [11](./11-type-modifications.md)–[13](./13-hidden-contracts.md) |
