# 第 24 章 · Calling and Closures（调用与函数） · §24.3 调用帧（Call Frames）

← [本章目录](./README.md) · 上一节：[02-function-objects.md](./02-function-objects.md) · 下一节：[04-function-calls.md](./04-function-calls.md)

---

递归 / 嵌套调用需要 **每调用一层一份状态**：

```c
typedef struct {
  ObjFunction* function;
  uint8_t* ip;           // 当前指令指针（指向 chunk->code 内）
  Value* slots;          // 本帧局部变量在 vm.stack 上的起始
} CallFrame;
```

| VM 字段 | 作用 |
|---------|------|
| **`frames[FRAMES_MAX]`** | CallFrame 数组 |
| **`frameCount`** | 当前深度 |

**`ip` 不再全局唯一**：每个 frame 有自己的 **`ip`**； **`vm.frameCount-1`** 为当前帧。

**`slots`**：指向 **该函数第一个局部槽** 在 **`vm.stack`** 中的位置（含参数）。

---
