# Item 7 · 易错细节

← [Item 7 目录](./README.md)

| 陷阱 | 原因 |
|------|------|
| `use of moved value` | 消费型 `build(self)` 后 builder 已失效 |
| **Temporary dropped while borrowed** | `&mut self` 链在**临时值**上，语句末临时对象 drop |
| 字段校验 | `build()` 里应集中校验（必填、互斥），别散落 setter |

---
