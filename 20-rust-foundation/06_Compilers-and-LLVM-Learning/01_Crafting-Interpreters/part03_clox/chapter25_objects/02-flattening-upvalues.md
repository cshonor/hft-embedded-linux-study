# 第 25 章 · Closures（闭包） · 扁平化 Upvalues（Flattening Upvalues）

← [本章目录](./README.md) · 上一节：[01-calling-and-closures-upvalue.md](./01-calling-and-closures-upvalue.md) · 下一节：[03-closing-upvalues.md](./03-closing-upvalues.md)

---

**嵌套**：

```lox
fun outer() {
  var x = 1;
  fun middle() {
    fun inner() { print x; }
    return inner;
  }
  return middle();
}
```

**规则**：`inner` 要读 `outer.x` → **不只** outer 捕获；**middle 也须捕获 upvalue 并向下传**，形成 **upvalue 链 / 扁平化索引**。

| 编译 | 说明 |
|------|------|
| **`resolveUpvalue(compiler, name)`** | 先在 **本函数 locals** 找；否则 **递归外层 Compiler** |
| 每层函数 | 维护自己的 **upvalue 列表**；内层引用外层 = **外层 upvalue 槽位传递** |

**效果**：任意嵌套深度，闭包总能 **O(1) 索引** 到正确捕获，无需运行时按名链式查找。

---
