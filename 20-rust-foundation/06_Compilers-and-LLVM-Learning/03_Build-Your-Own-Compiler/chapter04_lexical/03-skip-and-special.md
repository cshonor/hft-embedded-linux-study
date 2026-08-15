# 第 4 章 · 词法分析 · §3 扫描不生成 token 的单词

← [本章目录](./README.md) · 上一节：[02-token-unstructured.md](./02-token-unstructured.md) · 下一节：[04-structured-lexing-states.md](./04-structured-lexing-states.md)

---

## 为何要「跳过」

空白、注释 **不参与语法结构** — Parser 不应看到它们。

| 需求 | 命令 |
|------|------|
| 匹配并 **丢弃**，不出 token | **`SKIP`** |
| 跳过但 **可回溯访问** 跳过内容 | **`SPECIAL_TOKEN`** |

```text
SKIP      →  token 流中不可见
SPECIAL_TOKEN →  可通过 specialToken 链查看（如保留注释做工具）
```

C♭ 编译 **通常 SKIP 即可**；SPECIAL_TOKEN 为 IDE/文档工具预留。

---

## SKIP 典型规则

| 目标 | 概念规则 |
|------|----------|
| **空格 / 制表** | `" "`、`"\t"` |
| **换行** | `"\n"`、`"\r"` 等 |
| **行注释** | `"//" ~["\n","\r"]* ("\n"|"\r"|EOF?)` |

```text
// 行注释到换行结束 — 单状态 + 正则即可（无结构闭合问题）
```

→ 行注释属 **无配对结构**，与 §2 同类；块注释见 §4。

---

## SKIP vs TOKEN 分工

```text
  源码字符
     ↓
  [SKIP 规则]     空白、// 注释  →  不进入 token 流
     ↓
  [TOKEN 规则]    if、42、ident   →  Parser 消费
```

Parser **只读 TOKEN** — 词法层负责「静音」。

---

## 自测

- [ ] SKIP 与 SPECIAL_TOKEN 差异
- [ ] 行注释为何可不使用状态机
- [ ] Parser 为何不应直接读原始字符流
