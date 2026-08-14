# 第 24 章 · Calling and Closures（调用与函数） · §24.4～§24.5 函数调用（Function Calls）

← [本章目录](./README.md) · 上一节：[03-call-frames.md](./03-call-frames.md) · 下一节：[05-return-statements.md](./05-return-statements.md)

---

**`OP_CALL argCount`**（单字节参数个数）：

1. callee = stack 上 **argCount 个参数之下** 的那个 Value。
2. 必须是 **`ObjFunction` 或 `ObjNative`**。
3. 校验 **arity**。

### 栈重叠优化（核心）

调用前 caller 已：

```text
... | callee | arg1 | arg2 | ...   ← stackTop
```

**新 CallFrame**：

```text
frame.slots = callee 在栈上的位置
// arg1, arg2 恰好是 slots[1], slots[2] …
// 即「参数槽」= 被调函数「局部变量窗口」底部
```

| 效果 | 说明 |
|------|------|
| **零拷贝** | 参数传递 **不 memcpy** |
| 局部 **`var`** | 继续往 stack 上 grow，索引在 frame 内 |

**新帧**：`frame.ip = function->chunk.code`；`frameCount++`。

**对照**：真实 ABI 的 **栈传参**；LLVM 调用约定同样靠栈/寄存器窗口。

---
