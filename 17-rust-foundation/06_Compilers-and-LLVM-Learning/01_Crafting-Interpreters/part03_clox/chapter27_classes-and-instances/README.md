# 第 27 章 · Classes and Instances（类与实例）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/classes-and-instances.html) · [中文在线](https://craftinginterpreters.com/classes-and-instances.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

GC（ch26） 就绪后，安全引入 OOP：类是一等堆对象，调用类 = 构造实例；实例用 哈希表 存字段。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 类对象（Class Objects） | [01-class-objects.md](./01-class-objects.md) |
| ·3 | 类的实例化（Instances） | [02-instances.md](./02-instances.md) |
| ·4 | 字段与属性（Get and Set Expressions） | [03-get-and-set-expressions.md](./03-get-and-set-expressions.md) |
| ·5 | 对象模型（clox OOP 栈） | [04-clox-oop.md](./04-clox-oop.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
OP_CLASS     创建 ObjClass · 绑定类名
实例化       OP_CALL + ObjClass → ObjInstance
属性         OP_GET/SET_PROPERTY · instance.fields 哈希表
无 new       类当 callable
GC           mark 遍历 class/methods/fields
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **28** | [chapter28 · Methods](../chapter28_methods/) | 方法 · **`this`** · **`OP_INVOKE`** |
| **13** jlox | Inheritance | **`super`** · clox 后续章 |
| **25** 原书 | Objects | 与实例字段 opcode 重叠阅读 |

---

---

## 自测

1. `Pair()` 在字节码层与 `foo()` 如何区分 callee 类型？
2. 实例字段为何用 Table 而非固定 struct 字段？
3. GC mark 为何要扫描 `instance.fields` 里的 Value？

---

---

## 阅读进度

- [x] Class / Instance / Property 结构梳理（本章笔记）
- [ ] 对照 jlox ch12 画 clox opcode 路径
- [ ] 本章 + 原书 ch25 Challenges

