# 第 25 章 · Closures（闭包） · 闭包生命周期（总览）

← [本章目录](./README.md) · 上一节：[04-objclosure-op-closure.md](./04-objclosure-op-closure.md) · 下一节：---

```text
声明 outer 的 local x（栈槽）
  inner 捕获 → ObjUpvalue open → location = &stack[x]

outer 仍在执行：
  inner 读 x → 经 upvalue 读栈 ✓

outer return 前/块结束：
  OP_CLOSE_UPVALUE → x 值迁入堆上 ObjUpvalue

outer 已返回，栈回收：
  inner 仍持有 closed upvalue → 安全 ✓
```

---
