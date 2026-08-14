# 第 25 章 · Closures（闭包） · ObjClosure 与 OP_CLOSURE

← [本章目录](./README.md) · 上一节：[03-closing-upvalues.md](./03-closing-upvalues.md) · 下一节：[05-ast.md](./05-ast.md)

---

```text
compile fun:
  嵌套 Compiler → function chunk
  记录 upvalue 元数据（local slot / parent upvalue index）
  OP_CLOSURE constant(function) + upvalue count

run OP_CLOSURE:
  创建 ObjClosure + 填充 upvalues（open → 指向当前 frame slots）
  push closure Value
```

**调用闭包**：`OP_CALL` 目标为 **`ObjClosure`**（非裸 Function）→ 仍用 **CallFrame + chunk**，upvalues 挂在 closure 上。

---
