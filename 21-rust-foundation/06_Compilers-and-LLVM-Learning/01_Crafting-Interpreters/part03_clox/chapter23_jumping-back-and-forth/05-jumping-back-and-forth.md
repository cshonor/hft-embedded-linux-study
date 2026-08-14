# 第 23 章 · Jumping Back and Forth（来回跳转） · 跳转指令族（小结）

← [本章目录](./README.md) · 上一节：[04-for-statements.md](./04-for-statements.md) · 下一节：---

```text
OP_JUMP_IF_FALSE  条件假 → 向前跳过
OP_JUMP           无条件向前
OP_LOOP           无条件向后（循环）
        ↑ 偏移量在编译完成后 backpatch
```

---
