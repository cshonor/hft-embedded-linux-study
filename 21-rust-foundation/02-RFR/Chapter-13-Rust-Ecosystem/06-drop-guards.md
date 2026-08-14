# 2.2 Drop Guards（Drop 卫士）

> 所属：**Patterns in the Wild** · [← 章索引](./README.md)

在 **`unsafe`**、持锁区段或「必须配对释放」的资源上，构造局部 **`Guard`**，在 **`Drop`** 中清理。

- **panic 路径**仍会 `drop` — 恢复不变式、释放句柄
- Tokio 等库中常见

→ [第 1 章 · Drop](../Chapter-01-Foundations/04-ownership.md) · ER [Item 11 RAII](../../01-ER/Chapter-02-Traits/Item-11-drop-raii/README.md)
