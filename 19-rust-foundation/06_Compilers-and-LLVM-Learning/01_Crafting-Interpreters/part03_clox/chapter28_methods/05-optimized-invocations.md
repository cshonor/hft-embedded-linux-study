# 第 28 章 · Methods and Initializers（方法与初始化器） · §28.5 优化的调用（Optimized Invocations · Superinstruction）

← [本章目录](./README.md) · 上一节：[04-initializers.md](./04-initializers.md) · 下一节：[06-chunks-of-bytecode-chunk.md](./06-chunks-of-bytecode-chunk.md)

---

**模式**：源码 **`obj.method(args)`** —— 若拆成 **GET_PROPERTY + CALL** → 中间 **BoundMethod** 分配。

**窥孔优化（Peephole）**：Parser/Compiler 见到 **`.` 后紧跟 `(`** → 直接 **`OP_INVOKE`**。

| 指令 | 行为 |
|------|------|
| **`OP_INVOKE name argCount`** | **一次完成**：在 **instance** 上按名查 **方法** · 设置 **slot 0 = this** · **call** · **无 BoundMethod 分配** |

```text
// 慢路径
GET_PROPERTY "eat"
CALL 0          // BoundMethod 已堆分配

// 快路径
OP_INVOKE "eat" 0
```

**书中基准**：方法密集代码 **~7×+** 提升（相对 naive Get+Call）。

**概念**：**Superinstruction** = 融合多条语义等价 opcode 为一条 **更高层指令**（仍是一次 dispatch）。

---
