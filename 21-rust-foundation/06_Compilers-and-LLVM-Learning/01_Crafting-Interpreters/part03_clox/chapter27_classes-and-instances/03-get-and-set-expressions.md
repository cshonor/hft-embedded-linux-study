# 第 27 章 · Classes and Instances（类与实例） · 字段与属性（Get and Set Expressions）

← [本章目录](./README.md) · 上一节：[02-instances.md](./02-instances.md) · 下一节：[04-clox-oop.md](./04-clox-oop.md)

---

```lox
pair.first = 1;
print pair.first;
```

**编译**：

| 语法 | 字节码 |
|------|--------|
| **`obj.field`** | compile `obj` → **`OP_GET_PROPERTY`** name |
| **`obj.field = val`** | compile `obj` → compile `val` → **`OP_SET_PROPERTY`** name |

**运行时**：

```text
OP_GET_PROPERTY:
  instance = pop 必须是 ObjInstance
  name = constant string
  tableGet(instance.fields, name) → push

OP_SET_PROPERTY:
  value, instance = ...
  tableSet(instance.fields, name, value)
  push value（赋值表达式值）
```

| 设计 | 说明 |
|------|------|
| **每实例一个 Table** | 动态增删字段 · 类似 JS 对象 |
| **键 intern 字符串** | ch20 指针相等 · GC 弱引用表协同 |
| **方法 vs 字段** | ch28：**字段优先** 于同名方法 |

**对照 jlox ch12 §12.3～§12.4**：Get/Set AST → clox **opcode + 实例表**。

---
