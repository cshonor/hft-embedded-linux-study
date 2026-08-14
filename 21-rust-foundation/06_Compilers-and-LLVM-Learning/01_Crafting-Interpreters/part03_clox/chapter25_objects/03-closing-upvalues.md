# 第 25 章 · Closures（闭包） · 闭合 Upvalue（Closing Upvalues）

← [本章目录](./README.md) · 上一节：[02-flattening-upvalues.md](./02-flattening-upvalues.md) · 下一节：[04-objclosure-op-closure.md](./04-objclosure-op-closure.md)

---

**时机**：块/函数结束，**栈上被捕获的局部** 即将随 **`endScope` / `OP_RETURN`** 消失。

**编译**：在作用域结束前 emit **`OP_CLOSE_UPVALUE`**（对仍 open 的 upvalue）。

**运行时 `OP_CLOSE_UPVALUE`**：

```text
将 upvalue.location 指向的 Value 复制到 ObjUpvalue 内部（closed 存储）
location 改指向 closed 字段
该 upvalue 不再依赖栈
```

| 阶段 | upvalue.location 指向 |
|------|------------------------|
| **Open** | 栈上某 **`Value*`**（随 call frame 活） |
| **Closed** | **`ObjUpvalue` 内嵌 Value**（堆上，与帧无关） |

**外层已 return、栈已清空** 后，闭包仍通过 **closed upvalue** 安全读写的关键。

**VM 维护**： **`openUpvalues`** 链表（按栈位置排序），便于 scope 结束时 **批量 close**。

---
