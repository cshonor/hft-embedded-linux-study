# 第 20 章 · Hash Tables（哈希表） · 字符串驻留（String Interning）

← [本章目录](./README.md) · 上一节：[02-hash-functions-and-load-factor.md](./02-hash-functions-and-load-factor.md) · 下一节：[04-ast.md](./04-ast.md)

---

**问题**：哈希表频繁 **`strcmp`** 很慢。

**驻留**：全局表 **`tableSet(vm.strings, string, NULL)`**（或等价 API）：

| 规则 | 效果 |
|------|------|
| 创建字符串前 **查表** | 已存在 → **返回已有实例** |
| 不存在 | 分配新 **`ObjString`** 并插入 |
| 相等 | **`a == b`** ⇔ **指针相同**（同内容必同址） |

**编译器 / 运行时**：

- 变量名、属性名 → **intern 后的 `ObjString*`**。
- **`OP_EQUAL`** 对字符串可 **先比指针**（类型相同时）。

**对照**：Java **String pool** · Python **intern** · Lua string table。

---
