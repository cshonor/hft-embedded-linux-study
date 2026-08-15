# 第 14 章 · Chunks of Bytecode（字节码块） · Chunk 内存布局（小结）

← [本章目录](./README.md) · 上一节：[05-line-information.md](./05-line-information.md) · 下一节：---

```text
Chunk
├── code[]      Opcode + operands（主指令流）
├── lines[]     与 code 等长 · 源码行号
└── constants[] 常量池 · OP_CONSTANT 索引
```

---
