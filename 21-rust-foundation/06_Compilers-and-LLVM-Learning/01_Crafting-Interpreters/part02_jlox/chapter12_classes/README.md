# 第 12 章 · Classes（类）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/classes.html) · [中文在线](https://craftinginterpreters.com/classes.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

为 Lox 加入 基于类的 OOP：类声明、实例化、字段、方法、`this`、`init`。除 继承（ch13） 外，OOP 核心在此章齐备。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §12.1 | 类声明（Class Declarations） | [01-class-declarations.md](./01-class-declarations.md) |
| §12.2 | 创建实例（Creating Instances） | [02-creating-instances.md](./02-creating-instances.md) |
| §12.3 | 实例上的属性（Properties on Instances） | [03-properties-on-instances.md](./03-properties-on-instances.md) |
| §12.4 | 类上的方法（Methods on Classes） | [04-methods-on-classes.md](./04-methods-on-classes.md) |
| §12.5 | This | [05-this.md](./05-this.md) |
| §12.6 | 构造函数与初始化器（Constructors and Initializers） | [06-constructors-and-initializers.md](./06-constructors-and-initializers.md) |
| ·8 | OOP 能力矩阵（ch12 vs ch13） | [07-ch12-vs-ch13.md](./07-ch12-vs-ch13.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§12.1  Stmt.Class · LoxClass
§12.2  Breakfast() 无 new · LoxInstance
§12.3  Get/Set · instance.fields Map
§12.4  字段优先 · LoxBoundMethod
§12.5  this 环境 · Resolver 限制
§12.6  init 自动调用 · 禁 return 值
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **13** | [chapter13 · Inheritance](../chapter13_inheritance/) | **`super`** · 方法继承 |
| **14+** | [part03_clox](../../part03_clox/) | 字节码 VM 重新开始 |

---

---

## 自测

1. 实例字段与方法同名时，`obj.name` 解析到哪？
2. 为什么 `init` 要单独限制 `return`？
3. `LoxBoundMethod` 和裸 `LoxFunction` 在调用路径上差在哪一步？

---

---

## 阅读进度

- [x] §12.1～§12.6 结构梳理（本章笔记）
- [ ] 画 `bacon.cook()` 的 Get → Call → this 环境链
- [ ] 本章 Challenges

