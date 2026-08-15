# 第 10 章 · Functions（函数） · §10.1 函数调用（Function Calls）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-native-functions.md](./02-native-functions.md)

---

- 语法：`callee(arg1, arg2, …)`。
- **Parser**：把 **`()` 视为极高优先级的后缀运算符**（与 ch6 一元/二元分层衔接，call 在 call/column 层）。
- **Arity**：实参与形参个数必须一致，否则 **RuntimeError**。
- **255 参数上限**：与后续 **clox** 字节码格式对齐（单条指令操作数宽度），jlox 提前遵守。

**运行时抽象**：

| 接口 / 类 | 作用 |
|-----------|------|
| **`LoxCallable`** | 统一「可调用对象」：`call(Interpreter, List<Object> args)` + `arity()` |
| 校验 | 调用前比对 `args.size()` 与 `arity()` |

**AST**：`Expr.Call` —— callee 表达式 + arguments 列表。

---
