# 第 15 章 · A Virtual Machine（虚拟机） · §15.1 指令执行机（An Instruction Execution Machine）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-a-value-stack-manipulator.md](./02-a-value-stack-manipulator.md)

---

**`VM` 结构体**（核心字段）：

| 字段 | 作用 |
|------|------|
| **`chunk`** | 当前执行的 **`Chunk*`** |
| **`ip`** | **Instruction Pointer**，指向 **`chunk->code` 中下一条指令** |
| （§15.2）**`stack`** | 操作数 / 结果栈 |

**`interpret(chunk)`**：

```text
vm.chunk = chunk
vm.ip    = chunk->code   // 起始
return run()
```

**`run()` 骨架**：

```c
for (;;) {
  uint8_t instruction = *vm.ip++;
  switch (instruction) {
    case OP_CONSTANT: ... break;
    case OP_ADD:      ... break;
    ...
    case OP_RETURN:   return INTERPRET_OK;
  }
}
```

| 概念 | 说明 |
|------|------|
| **Fetch** | 读 `*ip++` 得 opcode |
| **Decode / Execute** | `switch` 分支（后续可换 **computed goto** 优化，ch30） |
| **循环** | 直到 **`OP_RETURN`** 或错误 |

**对照**：CPU 取指 → 译码 → 执行；VM 用 C **`switch`** 模拟 **dispatch table**。

---
