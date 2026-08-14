# 第 10 章 · 标量优化 · §2 核心优化技术与实例

← [本章目录](./README.md) · 上一节：[01-classification.md](./01-classification.md) · 下一节：[03-advanced-topics.md](./03-advanced-topics.md)

---

本章用精选例子覆盖**大部分标量转换种类** — 均在 ch9 **数据流 / SSA** 工具之上操作。

---

## 1. 消除无用与不可达代码

| 类型 | 定义 | 手段 |
|------|------|------|
| **无用代码** | 对程序结果**无影响**的语句 | **DCE**（Dead Code Elimination） |
| **不可达代码** | **永远不会执行**的基本块/分支 | 常量折叠控制流 → 删死区 |

**DCE 依赖**：[活跃变量分析](../chapter09_dataflow/01-iterative-dataflow.md) — 对 **LIVEOUT** 不贡献的 def 可删。

```text
x = a + b    // 若 x 此后不活跃 → 可删
if (false) { … }  // 不可达块 → 整段删
```

**LLVM**：`dce`、`adce`（Aggressive DCE）；`simplifycfg` 删不可达块。

---

## 2. 代码移动（Code Motion）

改变语句**执行位置**以减重复或提效。

| 变体 | 说明 |
|------|------|
| **LICM** | **Loop-Invariant Code Motion** — 循环不变式提到循环外 |
| **PRE / GCM** | 部分冗余消除 / 全局代码移动 — 更激进 |

```text
for (i…) {
  x = a * b;   // a,b 不变 → 提到循环前
  … use x …
}
```

**前提**：移动不得改变语义 — 需证明**无额外副作用**、**支配所有 use**。

---

## 3. 特化（Specialization）

据**上下文**把通用、高开销操作换成**特定场景**的低开销版本。

| 例子 | 通用 → 特化 |
|------|-------------|
| 常量已知 | `x * 8` → `x << 3` |
| 类型单态 | 泛型单态化后内联专用路径 |
| 分支已知 | `if (true)` 只留 then 臂 |

与 [ch8 值编号](../chapter08_intro-opt/02-redundant-expressions.md) 协同 — 特化**暴露**更多常量。

---

## 4. 冗余消除（Redundancy Elimination）

结合 ch8、ch9 数据流，消除**重复计算**。

| 技术 | 层级 | 链接 |
|------|------|------|
| **局部 CSE** | 基本块内 | ch8 DAG / VN |
| **全局 CSE** | 跨块 | ch8 **可用表达式** |
| **GVN / GCM** | SSA 上全局 | 等价类 + 代码移动 |

```text
t1 = a + b
…
t2 = a + b   →  t2 = t1   （或删 t2，复用 t1）
```

**SSA 优势**：单次赋值 → **use-def 链清晰**，GVN 更直接。

---

## 5. 激活其他转换（Enabling Transformations）

某些 Pass **本身不直接减时**，但**改结构**以**暴露**后续优化空间。

| 例子 | 激活效果 |
|------|----------|
| **内联** | 跨过程边界 → 块内 CSE、常量传播 |
| **循环展开** | 暴露更多 LICM / 向量化机会 |
| **SSA 构造** | 使 GVN、DCE 更简单 |
| **函数式化 / 去归纳变量** | 为强度减弱铺路 |

```text
Pass A（结构改写）  →  Pass B（冗余消除）  →  实际提速
     ↑ 工程上常故意先跑「铺路」Pass
```

---

## Pass 与数据流对照

| Pass | 常用分析 |
|------|----------|
| DCE | 活跃变量（反向） |
| CSE / GVN | 可用表达式 / 等价类 |
| 拷贝传播 | 到达定值 |
| LICM | 循环不变式 + 别名/内存分析 |

→ 分析细节见 [ch9](../chapter09_dataflow/README.md)。

---

## 自测

- [ ] DCE 与不可达代码删除各依赖什么
- [ ] LICM 一句话
- [ ] 举一个「激活型」Pass 例子
