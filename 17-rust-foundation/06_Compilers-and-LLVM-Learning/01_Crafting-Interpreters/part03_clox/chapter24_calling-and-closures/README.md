# 第 24 章 · Calling and Closures（调用与函数）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/calling-and-closures.html) · [中文在线](https://craftinginterpreters.com/calling-and-closures.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

此前 clox 仅在 顶层脚本 运行；本章起支持 `fun`、调用、递归、return、原生函数。函数体有 独立 Chunk + CallFrame；参数与局部变量共享 同一块栈窗口（零拷贝传递）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 闭包 / Upvalue（原书同章续） | [01-calling-and-closures-upvalue.md](./01-calling-and-closures-upvalue.md) |
| §24.1～ | §24.2 函数对象（Function Objects） | [02-function-objects.md](./02-function-objects.md) |
| §24.3 | 调用帧（Call Frames） | [03-call-frames.md](./03-call-frames.md) |
| §24.4～ | §24.5 函数调用（Function Calls） | [04-function-calls.md](./04-function-calls.md) |
| §24.6 | 返回语句（Return Statements） | [05-return-statements.md](./05-return-statements.md) |
| §24.7 | 原生函数（Native Functions） | [06-native-functions.md](./06-native-functions.md) |
| ·8 | Call 路径总览 | [07-call.md](./07-call.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§24.1–2 ObjFunction · chunk · arity
§24.3   CallFrame[] · ip + slots 每帧独立
§24.4–5 OP_CALL · 参数槽与 locals 窗口重叠
§24.6   OP_RETURN 弹帧 · 返回值压回 caller
§24.7   ObjNative · clock()
后续    [ch25 Closures](../chapter25_objects/README.md)
```

---

---

## 读后下一步

| 章 / 节 | 内容 |
|---------|------|
| **ch24 续** | [ch25 · Closures](../chapter25_objects/README.md) | **Upvalue** · **`OP_CLOSE_UPVALUE`** |
| **25** | [chapter25 · Objects](../chapter25_objects/) | 实例字段 |
| **26** | Garbage Collection | **mark-sweep** |

---

---

## 自测

1. 为什么 `CallFrame.slots` 指向 callee 而不是 arg1？
2. 递归时 `frameCount` 与 Java 调用栈的对应关系？
3. Native 与 Function 在 `OP_CALL` 里分歧在哪一步？

---

---

## 阅读进度

- [x] §24.1～§24.7 结构梳理（本章笔记）
- [ ] 画三次递归 `fact(n)` 的 frames 与 stack
- [ ] 继续阅读同章 Upvalue / 闭包小节
- [ ] 本章 Challenges

