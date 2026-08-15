# 第 3 章 · The Lox Language（Lox 语言概览） · §3.2 一种高级语言（A High-Level Language）

← [本章目录](./README.md) · 上一节：[01-hello-lox.md](./01-hello-lox.md) · 下一节：[03-data-types.md](./03-data-types.md)

---

Lox 定位接近 **JavaScript、Scheme、Lua** 等高级脚本语言。

### §3.2.1 动态类型（Dynamic typing）

| 项目 | 说明 |
|------|------|
| **含义** | 变量可存**任意类型**的值 |
| **检查时机** | 类型检查推迟到 **Runtime** |
| **实现影响** | jlox/clox 需运行时类型判别；clox 用 tagged value（**ch18**） |

**本仓库**：RFR 第 2 章静态类型与 layout · 与 Lox 动态模型对照读。

### §3.2.2 自动内存管理（Automatic memory management）

- 不手动 `malloc/free`。
- 依赖 **垃圾回收（GC）** 回收不再使用的内存。
- **jlox**：借 Java GC；**clox**：**ch26** 自实现 GC。

**本仓库**：RFR 第 1 章内存 · 第 9～10 章 · **Nomicon** 有效性。

---
