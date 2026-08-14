# 第 18 章 · Types of Values（值的类型） · 本章定位

← [本章目录](./README.md) · 下一节：[01-tagged-unions.md](./01-tagged-unions.md)

---

ch15～17 的 VM 本质是 **Unityped（仅数字）**；本章引入 **动态类型**：`number` · `bool` · `nil`，并为后续字符串、对象打基础。

| 对比 | ch15 Value | ch18 Value |
|------|------------|------------|
| 表示 | 裸 **`double`** | **Tagged union** |
| 字面量 | 数字 | **`true` / `false` / `nil`** |
| 运算 | 纯算术 | **truthy** · 比较 · 类型检查 |

| 主题 | 要点 |
|------|------|
| **Tagged Union** | `type` 标签 + **`union`** 载荷 |
| **Falsiness** | 仅 `nil`、`false` 为假 |
| **Equality / Compare** | `==` … · **`<=` 脱糖** |
| **Runtime Errors** | 算术前 **`checkNumber`** |

---
