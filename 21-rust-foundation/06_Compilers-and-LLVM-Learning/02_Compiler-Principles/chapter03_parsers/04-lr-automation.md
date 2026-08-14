# 第 3 章 · 语法分析 · §4 LR(1) 自动生成

← [本章目录](./README.md) · 上一节：[03-bottom-up-parsing.md](./03-bottom-up-parsing.md) · 下一节：[05-engineering-practice.md](./05-engineering-practice.md)

---

与 ch2「RE → DFA 识 Token」平行：ch3 用**自动机理论**识别语法**句柄**，构建 **LR(1) 表驱动分析器**。

---

## 构造 LR 自动机（DFA）

| 步骤 | 说明 |
|------|------|
| **项目（Item）** | 产生式中加「点」标记进度 — `A → α · β` |
| **闭包（Closure）** | 扩展项目集 — 若点在非终结符前，加入其产生式的项目 |
| **Goto** | 项目集在读到符号 X 后的**转移** → 新状态 |

得到跟踪分析进度的 **DFA**（状态 = 项目集）。

---

## Action 表与 Goto 表

| 表 | 决定什么 |
|----|----------|
| **Action[s, a]** | 状态 `s` 见输入 `a`：**移入** / **归约**（用哪条产生式）/ **接受** / **报错** |
| **Goto[s, X]** | 归约后/nonterminal 转移 — 状态 `s` 见非终结符 `X` 去下一状态 |

```text
while (true) {
  switch (Action[state, lookahead]) {
    case SHIFT:  push; advance;
    case REDUCE: pop; push Goto[...]; emit;
    case ACCEPT: return;
    case ERROR:  recover;
  }
}
```

**Bison** 输出即此类表 + 驱动代码。

---

## 与 ch2 对照

| ch2 扫描 | ch3 语法 |
|----------|----------|
| RE → NFA → DFA | CFG → LR 项目集 DFA |
| 转换表 `(state, char)` | Action/Goto `(state, token/nt)` |
| 识别 **Token** | 识别 **句柄**、构建结构 |

---

## 表规模问题（预告）

原始 **LR(1)** 表状态数可**极大** → [§5 表压缩 LALR/SLR](./05-engineering-practice.md)
