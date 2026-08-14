# 第 27 章 · Classes and Instances（类与实例） · 类的实例化（Instances）

← [本章目录](./README.md) · 上一节：[01-class-objects.md](./01-class-objects.md) · 下一节：[03-get-and-set-expressions.md](./03-get-and-set-expressions.md)

---

**无 `new`**：

```lox
var pair = Pair();
```

**`OP_CALL`** 路径扩展：

```text
callee = ObjClass
  → 分配 ObjInstance
  → instance.class = 该类
  → push(instance) 作为「构造结果」
  （init / 方法 ch28）
```

| 要点 | 说明 |
|------|------|
| **与函数共用 `OP_CALL`** | arity 校验 · CallFrame（`init` 时 ch28） |
| **`ObjInstance`** | 新堆对象 · **`fields` 空 Table** |

**对照 jlox [ch12 §12.2](../../part02_jlox/chapter12_classes/README.md)**：类作 **callable 工厂**。

---
