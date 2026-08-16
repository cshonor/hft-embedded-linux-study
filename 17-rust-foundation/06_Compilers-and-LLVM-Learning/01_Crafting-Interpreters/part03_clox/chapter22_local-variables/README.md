# 第 22 章 · Local Variables（局部变量）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/local-variables.html) · [中文在线](https://craftinginterpreters.com/local-variables.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch21 全局变量走 哈希表按名查找；ch22 引入 块作用域 + 局部变量，变量直接住在 Value Stack 槽位 上，读写 O(1) 按索引。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §22.1 | 表示局部变量（Representing Local Variables） | [01-representing-local-variables.md](./01-representing-local-variables.md) |
| §22.2～ | §22.3 块作用域与声明局部变量 | [02-22-3.md](./02-22-3.md) |
| §22.4 | 使用局部变量（Using Locals） | [03-using-locals.md](./03-using-locals.md) |
| §22.5 | 另一个作用域边缘情况（Another Scope Edge Case） | [04-another-scope-edge-case.md](./04-another-scope-edge-case.md) |
| ·6 | 局部 vs 全局（小结） | [05-local-variables-vs.md](./05-local-variables-vs.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§22.1  locals[] · scopeDepth · slot = 栈槽
§22.2–3 begin/endScope · 从后向前 resolve · shadowing
§22.4   OP_GET/SET_LOCAL 单字节索引
§22.5   var a=a · 未初始化 vs markInitialized
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **23** | [chapter23 · Jumping](../chapter23_jumping-back-and-forth/) | **回填** · `OP_JUMP` · 控制流 |
| **24** | Calling and Closures | **CallFrame** · **`OP_CALL`** |
| **11** jlox | Resolver | distance ↔ clox slot/upvalue |

---

---

## 自测

1. 为什么局部变量不需要放进常量池里的名字？
2. `endScope` 时编译器丢弃 locals，运行时栈如何同步变短？
3. `var a = a` 若不做两阶段，内层 `a` 解析会错成什么？

---

---

## 阅读进度

- [x] §22.1～§22.5 结构梳理（本章笔记）
- [ ] 手工 trace 嵌套 `{}` 的 locals 与 scopeDepth
- [ ] 本章 Challenges

