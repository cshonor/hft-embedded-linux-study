# 第 8 章 · 代码优化概述 · §5 过程间优化

← [本章目录](./README.md) · 上一节：[04-global-redundancy.md](./04-global-redundancy.md) · 下一节：---

**Whole-program** 层次 — 跨越 [ch6 调用边界](../chapter06_procedures/05-call-linkages.md)。

---

## 内联替换（Inline Substitution）

| 做法 | 把**被调函数体**复制到**调用点**，消除 call |
|------|---------------------------------------------|
| **收益** | 消 call/prologue/epilogue；暴露**更多上下文**给 Global 优化 |
| **代价** | 代码体积↑、I-Cache 压力、编译时间↑ |

**Rust**：`#[inline]` / `#[inline(always)]` · **LTO** 跨 crate 内联。

**HFT**：小热函数常 inline — 延迟敏感。

---

## 复制代码以增加上下文

| 手段 | 说明 |
|------|------|
| **克隆 + 特化** | 针对某调用上下文 duplicate 函数体 |
| **常量传播进被调** | 实参为常 → 特化版本 |
| **去虚化（Devirtualization）** | 确定接收者类型 → 直接 call 具体方法 |

与 [ch7 §5 OOP dispatch](../chapter07_code-shape/05-calls-and-oop.md) 联动。

---

## 其他过程间话题（预告）

| 优化 | 作用 |
|------|------|
| **IPO / Interprocedural analysis** | 跨函数传播常量、别名 |
| **LTO / ThinLTO** | 链接期整程序优化 |
| **Outline vs Inline** | 体积与速度权衡 |

---

## 本章收束

| 主题 | 落点 |
|------|------|
| **作用域分层** | 优化器架构语言 |
| **VN / DAG / 可用表达式** | 冗余消除主线 |
| **内联** | 过程间入口 |

→ **ch9** 数据流分析形式化 · **ch10** SSA 上具体标量 Pass

---

## 与 CI ch30

clox **教学向**局部优化（如 NaN boxing、指令特化）— 非工业全局数据流，但体现「不改语义改形态」→ [ch30](../../../01_Crafting-Interpreters/part03_clox/chapter30_optimization/README.md)
