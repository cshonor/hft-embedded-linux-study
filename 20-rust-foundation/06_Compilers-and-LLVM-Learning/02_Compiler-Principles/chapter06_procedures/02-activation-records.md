# 第 6 章 · 过程抽象 · §2 活动记录（Activation Record）

← [本章目录](./README.md) · 上一节：[01-control-and-name-space.md](./01-control-and-name-space.md) · 下一节：[03-addressability.md](./03-addressability.md)

---

## 为何需要 AR

同一函数可被**多次、递归**调用 — 每次调用需要**独立**的局部状态 → **活动记录（AR / Stack Frame）**。

---

## AR 中通常有什么

| 字段 | 作用 |
|------|------|
| **局部变量** | 本调用的局部数据 |
| **参数** | 实参（或与参数区共享栈窗口） |
| **返回地址** | Return 跳回哪里 |
| **保存的寄存器** | 被调用者需 preserved 的 callee-saved |
| **调用者 AR 指针** | 链到外层帧（动态链 dynamic link） |
| **（可选）存取链** | 嵌套过程寻址 — [§3](./03-addressability.md) |

---

## 分配：栈（Stack）

| 方式 | 说明 |
|------|------|
| **栈上分配 AR** | **LIFO** — 调用 push 帧，返回 pop 帧 |
| **效率** | 分配/释放 O(1)；缓存友好 |

```text
高地址
  ┌─────────────┐
  │  caller AR  │
  ├─────────────┤
  │  callee AR  │  ← 当前 SP/FP
  ├─────────────┤
  │    …        │
低地址
```

**Rust 默认**：栈帧 + 寄存器；`async` 状态机可把帧 lowering 到堆 — RFR 第 8 章。

---

## 与 clox CallFrame

clox **CallFrame** = 简化 AR：指向 **Chunk**、**IP**、**栈槽窗口**（locals + args 共享）。

→ [ch24 Call Frames](../../../01_Crafting-Interpreters/part03_clox/chapter24_calling-and-closures/03-call-frames.md)

**HFT**：热路径函数调用成本 = prologue/epilogue + 参数传递 + **ICache** — 见 [§5](./05-call-linkages.md)。
