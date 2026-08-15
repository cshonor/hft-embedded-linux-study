# 第 21 章 · Global Variables（全局变量） · 全局变量声明（Variable Declarations）

← [本章目录](./README.md) · 上一节：[01-statements.md](./01-statements.md) · 下一节：[03-reading-and-assignment.md](./03-reading-and-assignment.md)

---

```lox
var breakfast = "bagels";
var lunch;    // → nil
```

**编译 `var name = init;`**：

1. 若有 init → compile initializer；否则 **`OP_NIL`**（或 constant nil）。
2. 变量名 lexeme → **`identifierConstant`** → **intern `ObjString*`** → 常量池索引。
3. **`OP_DEFINE_GLOBAL`** + 名字常量索引。

**VM 执行 `OP_DEFINE_GLOBAL`**：

```text
name = readString(constant)
value = pop()
tableSet(&vm.globals, name, value)
```

**顶层重复 `var`**：允许（与 jlox 一致）→ **覆盖** 全局绑定。

---
