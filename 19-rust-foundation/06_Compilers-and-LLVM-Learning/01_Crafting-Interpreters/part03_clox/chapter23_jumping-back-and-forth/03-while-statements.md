# 第 23 章 · Jumping Back and Forth（来回跳转） · §23.3 While 循环（While Statements）

← [本章目录](./README.md) · 上一节：[02-logical-operators.md](./02-logical-operators.md) · 下一节：[04-for-statements.md](./04-for-statements.md)

---

```lox
while (cond) body;
```

```text
loopStart:
  compile cond
  JUMP_IF_FALSE → exit
  compile body
  OP_LOOP → loopStart   // 往回跳
exit:
```

**`OP_LOOP` offset**：

- **负向/backward**：从当前 ip **回跳** offset 字节到 **loop 头**。
- 与 **`OP_JUMP`（向前）** 配对：条件失败 **向前跳出**； body 结束 **向后循环**。

**VM**：`ip -= offset`（实现细节以书中为准）。

---
