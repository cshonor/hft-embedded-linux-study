# 第 23 章 · Jumping Back and Forth（来回跳转） · §23.4 For 循环（For Statements）

← [本章目录](./README.md) · 上一节：[03-while-statements.md](./03-while-statements.md) · 下一节：[05-ast.md](./05-ast.md)

---

C 风格 **`for (init; cond; increment) body`** → **前端脱糖**，**无专属 `OP_FOR`**。

典型 lowering（与 jlox ch9 §9.5 同构）：

```text
init
loopStart:
  cond → JUMP_IF_FALSE exit
  body
  increment
  OP_LOOP loopStart
exit:
```

| 优点 | 说明 |
|------|------|
| VM 简单 | 只需 jump 族 + loop |
| 与 jlox 一致 | 语法糖不进入 IR |

**init 作用域**：若 init 是 **`var`**，整个 for 外层包 **`beginScope/endScope`**（变量作用域限于 for）。

---
