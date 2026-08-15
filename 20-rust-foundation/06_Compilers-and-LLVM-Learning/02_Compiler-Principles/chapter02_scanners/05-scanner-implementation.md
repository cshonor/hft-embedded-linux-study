# 第 2 章 · 扫描 · §5 工程实现（表驱动 vs 直接编码）

← [本章目录](./README.md) · 上一节：[04-re-to-dfa-pipeline.md](./04-re-to-dfa-pipeline.md) · 下一节：[06-keywords-and-tradeoffs.md](./06-keywords-and-tradeoffs.md)

---

最小化 DFA 就绪后，落地为代码有两种主流方式：

---

## 1. 表驱动扫描器（Table-driven Scanner）

| 组件 | 作用 |
|------|------|
| **二维转换表** | `(当前状态, 输入字符) → 下一状态` |
| **驱动循环** | 简单 `while` 读字符、查表、转移 |
| **接受状态表** | 标记哪些状态对应何种 Token |

**优点**：结构清晰、易由工具自动生成、易调试。  
**缺点**：每次转移需**读表** — 内存访问开销。

---

## 2. 直接编码扫描器（Direct-coded Scanner）

- **不用转换表**。
- 将 DFA 状态图变为 **`switch` / `goto` + 条件判断** 的控制流。
- 每个状态 ≈ 一段代码块。

**优点**：避免查表，**通常更快**（热点路径友好，利于 CPU 分支预测）。  
**缺点**：代码体积大；手写维护成本高（生成器更常产出表驱动，但可优化为 direct-coded）。

---

## 工程权衡（ch1 Trade-offs 再现）

| 维度 | 表驱动 | 直接编码 |
|------|--------|----------|
| 速度 | 稍慢（查表） | 通常更快 |
| 体积 | 表占内存 | 代码段大 |
| 生成/维护 | flex 默认友好 | 需专门代码生成策略 |

**HFT / Rust 联想**：编译器自身扫描速度影响**编译延迟**；生成的目标程序扫描（若嵌入 DSL）则影响**运行时**。热点路径常选 direct-coded 或手工 lexer（如 CI clox）。

---

## 与 CI 对照

[jlox ch4](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/README.md) / [clox ch16](../../../01_Crafting-Interpreters/part03_clox/chapter16_scanning-on-demand/README.md) 的 `Scanner` ≈ **人脑设计的 direct-coded** 风格（`switch` on 字符 / 状态机逻辑），未走 flex 生成。
