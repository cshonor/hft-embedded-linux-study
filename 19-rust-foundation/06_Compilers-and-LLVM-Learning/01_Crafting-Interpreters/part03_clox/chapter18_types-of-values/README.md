# 第 18 章 · Types of Values（值的类型）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/types-of-values.html) · [中文在线](https://craftinginterpreters.com/types-of-values.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch15～17 的 VM 本质是 Unityped（仅数字）；本章引入 动态类型：`number` · `bool` · `nil`，并为后续字符串、对象打基础。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 标记联合（Tagged Unions） | [01-tagged-unions.md](./01-tagged-unions.md) |
| ·3 | 真假值与逻辑非（Falsiness and Logical Not） | [02-falsiness-and-logical-not.md](./02-falsiness-and-logical-not.md) |
| ·4 | 相等与比较（Equality and Comparison） | [03-equality-and-comparison.md](./03-equality-and-comparison.md) |
| ·5 | 运行时错误（Runtime Errors） | [04-runtime-errors.md](./04-runtime-errors.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
Value     type tag + union · IS_/AS_/_VAL 宏
Falsiness nil/false 假 · OP_NOT
Compare   <= >= 脱糖为 GREATER + NOT
Runtime   checkNumber · runtimeError + line
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **19** | [chapter19 · Strings](../chapter19_strings/) | **`Obj`** · **`ObjString`** |
| **20** | Hash Tables | 开放寻址 · **intern** |
| **22** | Local Variables | 局部 slot · 非仅全局 |

---

---

## 自测

1. 为什么 `0` 在 Lox 里是 truthy？
2. `<=` 脱糖后 VM 需要几条指令？比单独 `OP_LESS_EQUAL` 多什么？
3. Tagged union 与 jlox 的 `Object` 各适合什么运行时？

---

---

## 阅读进度

- [x] Tagged union / falsiness / 比较 / 运行时错误 结构梳理（本章笔记）
- [ ] 列出 `ValueType` 扩展路径（string/obj）
- [ ] 本章 Challenges

