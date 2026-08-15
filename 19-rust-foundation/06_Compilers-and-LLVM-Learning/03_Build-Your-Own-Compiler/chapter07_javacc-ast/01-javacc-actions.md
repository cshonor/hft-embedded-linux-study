# 第 7 章 · JavaCC 的 action 和抽象语法树 · §1 JavaCC 的 action

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-ast-and-nodes.md](./02-ast-and-nodes.md)

---

## action 的作用

| 仅文法 | + action |
|--------|----------|
| **语法检查** — 合法/非法 | **解析 + 建语法树** |

在识别出语句、表达式等时 **执行 Java 代码** — 嵌入规则，用 **`{ … }`** 包裹。

```text
// 概念
ExprNode expr() :
{ ExprNode lhs, rhs; Token op; }
{
  lhs = expr10()
  [ op = <ASSIGN> rhs = expr() { return new AssignNode(lhs, rhs); } ]
  { return lhs; }
}
```

匹配到符号串 → **执行** `{}` 内代码。

---

## 语义值与 Token

**每个符号**（终端 / 非终端）可有 **语义值**。

| 符号 | 语义值来源 |
|------|------------|
| **终端符** | JavaCC 自动生成 **`Token`** 实例 |
| **非终端符** | action 里 **`return`** 的对象（如 AST 节点） |

**`Token` 常用字段**：

| 属性 | 含义 |
|------|------|
| **`image`** | 匹配到的 **字面文本** |
| **`beginLine` / `beginColumn`** 等 | **源码位置** — 报错用 |

```text
t = <IDENTIFIER>  →  t.image 为变量名字符串
```

---

## 获取与返回语义值

| 操作 | 写法（概念） |
|------|--------------|
| **取终端值** | `变量 = <TOKEN>` |
| **取非终端值** | `变量 = 非终端()` 的返回值 |
| **返回给上层** | `return node;` — 赋给左侧非终端 |

```text
ExprNode term() :
{ Token t; }
{
  t = <INTEGER> { return new IntNode(t); }
}
```

产生式 **整体返回值类型** 在 JavaCC 中声明为 `ExprNode expr()` 等。

---

## 执行时机与模式组合

action **不限于规则末尾** — 可写在 **任意位置**。

| 组合 | 行为 |
|------|------|
| **`\|` 选择** | 各分支内独立 `{}` — 匹配哪支执行哪支 |
| **`*` / `+` 重复** | `{}` 放在 **重复括号内** → **每匹配一次执行一次** |

```text
// 概念：参数列表
( arg = expr() { args.add(arg); } )*
```

**用途**：累加列表、循环构建 `List<Node>` — ch8 声明/语句列表常用。

---

## 与 ch6 衔接

ch6 **空 action** 或仅结构；ch7 起同一产生式 **填 `return new …Node`**。

---

## 自测

- [ ] 无 action 时 parser 能做什么、不能做什么
- [ ] Token 的 `image` 与 `beginLine` 各用于什么
- [ ] 重复模式内 action 何时多次执行
