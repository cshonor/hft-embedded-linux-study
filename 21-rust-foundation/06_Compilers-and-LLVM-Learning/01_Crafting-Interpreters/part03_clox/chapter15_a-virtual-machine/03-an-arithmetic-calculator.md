# 第 15 章 · A Virtual Machine（虚拟机） · §15.3 算术计算器（An Arithmetic Calculator）

← [本章目录](./README.md) · 上一节：[02-a-value-stack-manipulator.md](./02-a-value-stack-manipulator.md) · 下一节：[04-vm-chunk.md](./04-vm-chunk.md)

---

实现 opcode（与 ch14 枚举对应）：

| 指令 | 行为 |
|------|------|
| **`OP_CONSTANT idx`** | 读 1 字节索引 → 从常量池取 **Value** → **push** |
| **`OP_NEGATE`** | pop 一个 → 取负 → push |
| **`OP_ADD` / `SUBTRACT` / `MULTIPLY` / `DIVIDE`** | pop 右、pop 左 → 运算 → push |
| **`OP_RETURN`** | 结束；栈顶为结果（调试时可 print） |

**执行示例** `-((1.2 + 3.4) / 5.6)`（概念栈变化）：

```text
CONSTANT 1.2    stack: 1.2
CONSTANT 3.4    stack: 1.2 3.4
ADD             stack: 4.6
CONSTANT 5.6    stack: 4.6 5.6
DIVIDE          stack: 0.821...
NEGATE          stack: -0.821...
RETURN
```

| 优雅之处 | 说明 |
|----------|------|
| 编译器 | 按表达式结构 **emit 序列**，无需寄存器分配 |
| VM | 每条指令 **O(1)** 栈操作 |

**除零等**：运行时检查 → 报错 + **行号**（ch14 **`lines[]`**）。

**仍缺**：变量、控制流、函数、对象——**ch17+** 逐步加 opcode。

---
