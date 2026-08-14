# 第 23 章 · Jumping Back and Forth（来回跳转） · §23.2 逻辑运算符（Logical Operators）

← [本章目录](./README.md) · 上一节：[01-if-statements.md](./01-if-statements.md) · 下一节：[03-while-statements.md](./03-while-statements.md)

---

Lox：**短路** + **返回决定结果的原始值**（不一定是 bool）。

**`and`**（左 falsy → 整式 = 左）：

```text
compile left
JUMP_IF_FALSE → end        // 左 falsy：栈顶已是结果
POP                        // 左 truthy：丢弃，继续
compile right
end:
```

**`or`**（左 truthy → 整式 = 左）：

```text
compile left
JUMP_IF_FALSE → evalRight  // 左 falsy 才算右
JUMP → end                 // 左 truthy：栈顶保留
evalRight:
POP
compile right
end:
```

| 要点 | 说明 |
|------|------|
| 复用 **`OP_JUMP_IF_FALSE`** | 少 opcode |
| 栈顶 | 短路时 **左操作数留在栈上** 作为结果 |

**对照 jlox ch9 §9.3**：语义相同；实现从 Visitor 改为 **控制流 bytecode**。

---
