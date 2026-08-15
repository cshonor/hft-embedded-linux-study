# 第 14 章 · Chunks of Bytecode（字节码块） · 本章定位

← [本章目录](./README.md) · 下一节：[01-what-is-bytecode.md](./01-what-is-bytecode.md)

---

**Part III 奠基**：在造 VM 之前，先定义 **字节码如何存放**。clox 用 C 手写 **紧凑指令序列 + 常量池 + 行号表**，告别 jlox 的 AST 指针树。

| 对比 | jlox (Part II) | clox (Part III) |
|------|----------------|-----------------|
| 程序表示 | **AST** 节点树 | **`Chunk`** 字节数组 |
| 执行 | Visitor 递归遍历 | **VM** 读 ip  dispatch |
| 语言 | Java | **C** |

| 小节 | 主题 |
|------|------|
| **§14.1** | 何为 **Bytecode** · 相对 AST / 机器码 |
| **§14.3** | **`Chunk`** · 动态字节数组 · Opcode |
| **§14.4** | **反汇编** · debug 黑盒 |
| **§14.5** | **常量池** · `OP_CONSTANT` |
| **§14.6** | **行号** 平行数组 |

（原文 **§14.2** 多为从 jlox 过渡到 clox 的承上启下，与 §14.1 连贯阅读即可。）

---
