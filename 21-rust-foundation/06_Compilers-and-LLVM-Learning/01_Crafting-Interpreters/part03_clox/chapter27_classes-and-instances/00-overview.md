# 第 27 章 · Classes and Instances（类与实例） · 本章定位

← [本章目录](./README.md) · 下一节：[01-class-objects.md](./01-class-objects.md)

---

**GC（ch26）** 就绪后，安全引入 **OOP**：类是一等堆对象，**调用类 = 构造实例**；实例用 **哈希表** 存字段。

| 对比 | jlox ch12 | clox ch27 |
|------|-----------|-----------|
| 类 | **`LoxClass`** | **`ObjClass`** |
| 实例化 | **`Breakfast()`** | **`OP_CALL`** 判别 **`ObjClass`** |
| 字段 | **`LoxInstance.fields`** | **`ObjInstance.fields` Table** |
| 语法 | **`Expr.Get/Set`** | **`OP_GET/SET_PROPERTY`** |

| 主题 | 要点 |
|------|------|
| **Class Objects** | **`OP_CLASS`** · **`ObjClass`** |
| **Instances** | 无 **`new`** · call class |
| **Properties** | **`.`** · 实例 **Table** |

---
