# 第 10 章 · 标量优化 · 本章定位

← [本章目录](./README.md) · 上一章：[ch9 数据流分析](../chapter09_dataflow/README.md) · 下一节：[01-classification.md](./01-classification.md)

---

| ch8 | ch9 | ch10 |
|-----|-----|------|
| **优化地图**（作用域、VN） | **分析引擎**（数据流、SSA 构建） | **兵器谱**（在 SSA 上跑具体 Pass） |

```text
ch8  「消什么冗余、作用域多大」
  ↓
ch9  「怎么算 LIVE / 可用 · 怎么建 SSA」
  ↓
ch10 「常量传播、DCE、LICM、CSE、强度减弱…」
  ↓
ch11 「选指令 — 机器相关」
```

**核心**：把程序分析的抽象理论变成**可执行的代码转换** — 现代优化编译器的实践阶段。

**前提 IR**：多数 Pass 在 [ch9 构建的 SSA](../chapter09_dataflow/README.md) 上效果最好。
