# 第 27 章 · Classes and Instances（类与实例） · 对象模型（clox OOP 栈）

← [本章目录](./README.md) · 上一节：[03-get-and-set-expressions.md](./03-get-and-set-expressions.md) · 下一节：---

```text
ObjClass (methods Table)
    │
    │ OP_CALL / 工厂
    ▼
ObjInstance (fields Table)
    │
    │ OP_GET/SET_PROPERTY
    ▼
  动态字段读写
```

**GC**：**`markClass` / `markInstance`** 标记 **name、methods、fields** 中 Value 引用的 Obj。

---
