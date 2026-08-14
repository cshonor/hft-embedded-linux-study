# 第 22 章 · Local Variables（局部变量） · §22.4 使用局部变量（Using Locals）

← [本章目录](./README.md) · 上一节：[02-22-3.md](./02-22-3.md) · 下一节：[04-another-scope-edge-case.md](./04-another-scope-edge-case.md)

---

| 指令 | 操作 |
|------|------|
| **`OP_GET_LOCAL slot`** | `push(stack[slot])`（相对当前 CallFrame 底，ch24 前为单一顶层帧） |
| **`OP_SET_LOCAL slot`** | `stack[slot] = peek()` |

| 优势 | 说明 |
|------|------|
| **单字节 slot** | 无字符串 · 无 hash |
| **O(1)** | 直接索引 VM 栈 |
| **缓存友好** | 栈数组连续访问 |

**标识符编译**：

```text
resolveLocal → emit OP_GET_LOCAL index
canAssign + '=' → emit OP_SET_LOCAL index
未解析到局部 → OP_GET/SET_GLOBAL（ch21）
```

---
