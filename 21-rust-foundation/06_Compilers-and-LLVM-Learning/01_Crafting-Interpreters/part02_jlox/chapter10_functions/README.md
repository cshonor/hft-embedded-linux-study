# 第 10 章 · Functions（函数）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/functions.html) · [中文在线](https://craftinginterpreters.com/functions.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch9 已图灵完备；ch10 加入 函数 → 代码复用、参数传递、闭包（§10.6 初版，ch11 修 Bug）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §10.1 | 函数调用（Function Calls） | [01-function-calls.md](./01-function-calls.md) |
| §10.2 | 原生函数（Native Functions） | [02-native-functions.md](./02-native-functions.md) |
| §10.3 | 函数声明（Function Declarations） | [03-function-declarations.md](./03-function-declarations.md) |
| §10.4 | 函数对象（Function Objects） | [04-function-objects.md](./04-function-objects.md) |
| §10.5 | 返回语句（Return Statements） | [05-return-statements.md](./05-return-statements.md) |
| §10.6 | 局部函数与闭包（Local Functions and Closures） | [06-local-functions-and-closures.md](./06-local-functions-and-closures.md) |
| ·8 | 三阶段能力小结（ch8～10） | [07-ch8-10.md](./07-ch8-10.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§10.1  Call 后缀 · LoxCallable · arity · ≤255 args
§10.2  Native · clock()
§10.3  Stmt.Function · fun 解析
§10.4  LoxFunction · 每次 call 新 Environment
§10.5  Return 异常跳出 Visitor 栈
§10.6  closure 捕获定义时环境
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **11** | [chapter11 · Resolving](../chapter11_resolving-and-binding/) | **Resolver** · 静态作用域 · distance |
| **12** | [chapter12 · Classes](../chapter12_classes/) | `class` · `this` · OOP |

---

---

## 自测

1. 为什么 `()` 解析成「后缀运算符」而不是独立语句？
2. `return` 为什么用异常而不是改 Visitor 返回值类型？
3. 闭包里的 `enclosing` 指向哪里——定义处还是调用处？

---

---

## 阅读进度

- [x] §10.1～§10.6 结构梳理（本章笔记）
- [ ] 实现 / 对照 jlox 源码：`LoxFunction.call`、`Return`
- [ ] 本章 Challenges

