# 第 15 章 · A Virtual Machine（虚拟机） · VM 与 Chunk 协作

← [本章目录](./README.md) · 上一节：[03-an-arithmetic-calculator.md](./03-an-arithmetic-calculator.md) · 下一节：---

```text
         ┌─────────────┐
         │   Chunk     │
         │ code[]      │◄── ip 逐字节前进
         │ constants[] │
         │ lines[]     │──► 报错行号
         └─────────────┘
                ▲
                │ fetch/decode
         ┌──────┴──────┐
         │     VM      │
         │  stack[]    │  中间结果 LIFO
         └─────────────┘
```

---
