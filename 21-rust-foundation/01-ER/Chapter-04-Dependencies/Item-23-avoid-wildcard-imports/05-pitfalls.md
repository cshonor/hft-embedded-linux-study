# Item 23 · 易错细节

← [Item 23 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **忽略 Clippy** | glob import 有相关 lint（Item 29）；图省事 = 埋定时炸弹 |
| **以为本地优先万能** | 只救**类型名**；**trait 方法**仍歧义 |
| **`cargo update` 无感 break** | 依赖范围未锁 Major，Minor 加 trait 即可炸 build |

---
