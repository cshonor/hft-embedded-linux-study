# 第 5 章 · 中间表示 · §3 图示 IR

← [本章目录](./README.md) · 上一节：[02-ir-taxonomy.md](./02-ir-taxonomy.md) · 下一节：[04-linear-ir.md](./04-linear-ir.md)

---

## 抽象语法树（AST）

| 项目 | 说明 |
|------|------|
| **来源** | 语法分析树的**精简版** |
| **去掉** | 无语义标点、多余非终结符 |
| **保留** | 源程序**语法嵌套**关系 |

→ [ch4 SDT 建 AST](../chapter04_context/04-syntax-directed-translation.md) · [CI jlox ch5](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/README.md)

**jlox** 以 AST 为「IR」直接解释执行。

---

## 有向无环图（DAG）

| 相对 AST | 说明 |
|----------|------|
| **共享子表达式** | 相同子树 → **同一节点** |
| **效果** | 更紧凑；隐含**公共子表达式消除（CSE）** |

```text
AST:        (+)          DAG:      (+)
           /   \                   / \
          a    (+)                a   b
              / \
             b   b    ← 重复      （b 只出现一次）
```

---

## 控制流图（CFG）

表示**程序执行路径**。

| 成分 | 含义 |
|------|------|
| **节点** | **基本块（Basic Block）** — 无内部分支的**直线代码** |
| **边** | 块间控制转移 — `if` 分支、`while` 回边、`return` 等 |

```text
       ┌──► [BB2: then 部分] ──┐
[BB1] ─┤                       ├──► [BB4: 汇合]
       └──► [BB3: else 部分] ──┘
```

**优化入口**：循环检测、块内/local opt、全局数据流 — 多在 CFG 上定义。

→ RFR 第 8 章 async 状态机也可视作 CFG 变体

---

## 图示 IR 小结

| IR | 擅长表达 |
|----|----------|
| **AST** | 语法结构、递归下降 lowering |
| **DAG** | 表达式共享、CSE |
| **CFG** | 控制流、过程间/循环优化 |
