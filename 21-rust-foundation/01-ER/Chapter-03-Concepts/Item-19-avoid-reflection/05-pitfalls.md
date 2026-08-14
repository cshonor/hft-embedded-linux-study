# Item 19 · 易错细节

← [Item 19 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **解析 `type_name` 字符串** | 非稳定契约；同名路径不保证唯一 → 用 **`TypeId`** |
| **trait 对象 upcasting** | 1.70 前 `dyn Shape`（`Shape: Draw`）不能平滑转 `dyn Draw` — 虚表布局不同 |
| **指望 Any 当通用反射** | 无 supertrait；upcasting 也帮不了 Any |

> **演进注**：Rust **1.76+** 稳定 **trait upcasting**（`dyn Shape` → `dyn Draw`）。但 **`Any` 无父 trait**，新能力**不增加** Any 的反射能力。

---
