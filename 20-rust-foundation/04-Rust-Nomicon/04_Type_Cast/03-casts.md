# 3 · 显式转换 (Casts)

← [本章目录](./README.md) · 上一节：[02-dot-operator.md](./02-dot-operator.md) · 下一节：[04-transmutes.md](./04-transmutes.md)

---

`expr as Type` — 隐式转换的**超集**。

| 范围 | 说明 |
|------|------|
| 基本数字 | `i32 as u8` 等，可能截断/改变符号 |
| 原生指针 | 运行时通常不报错，但随意 cast 是 UB 温床 |
| **非传递性** | `e as U1 as U2` 合法 **≠** `e as U2` 合法 |

→ 源码：[src/casts.rs](./src/casts.rs)
