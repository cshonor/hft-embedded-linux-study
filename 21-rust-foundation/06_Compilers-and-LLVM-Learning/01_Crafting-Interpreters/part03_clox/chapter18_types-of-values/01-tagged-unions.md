# 第 18 章 · Types of Values（值的类型） · 标记联合（Tagged Unions）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-falsiness-and-logical-not.md](./02-falsiness-and-logical-not.md)

---

C 中用统一 **`Value`** 表示 Lox 任意值：

```c
typedef struct {
  ValueType type;
  union {
    double number;
    bool boolean;
  } as;
} Value;
```

| 宏族 | 用途 |
|------|------|
| **`IS_NUMBER(v)`** / **`IS_BOOL`** / **`IS_NIL`** | 类型判别 |
| **`AS_NUMBER(v)`** 等 | 解包（前提已检查） |
| **`NUMBER_VAL(x)`** / **`BOOL_VAL`** / **`NIL_VAL`** | 构造 Value |

**`ValueType` 枚举**：`VAL_nil` · `VAL_bool` · `VAL_number`（字符串/对象类型在 ch19+ 扩展）。

**对照 jlox [ch7](../../part02_jlox/chapter07_evaluating-expressions/README.md)**：Java **`Object`** 装箱；clox **显式 tag**，无 GC 压力于小值。

**注**：部分 VM 用 **NaN boxing** 把 tag 塞进 double 位型；本书 ch18 先用 **清晰 struct**，易读易 debug（与目录「NaN tagging」为后续优化方向，非本章必实现）。

---
