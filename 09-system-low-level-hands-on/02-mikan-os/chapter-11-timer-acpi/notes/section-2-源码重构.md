## 2. 源码重构

> 功能增多 — **`main.cpp` 臃肿** · 手写队列可换标准库。

---

### 一、InitializeXXX 拆分

将 **KernelMain** 中初始化 **按子系统拆函数**：

| 函数（示意） | 职责 |
|--------------|------|
| **`InitializeGraphics()`** | GOP · LayerManager · Back Buffer |
| **`InitializeInterrupt()`** | IDT · MSI · APIC |
| **`InitializeMemory()`** | Memory map · 分页 · Bitmap |
| **`InitializeTimer()`** | APIC Timer · TimerManager |
| **`InitializeACPI()`** | RSDP · PM Timer |

| 收益 | 说明 |
|------|------|
| **可读** | main 只保留 **调用顺序** |
| **调试** | 失败点定位到 **单一 Initialize** |
| **与 Linux initcall** | 同构（缩小版） |

---

### 二、std::deque 替换手写队列

Ch7 **`ArrayQueue`** 定长 FIFO — 本章部分场景改用 **`std::deque`**：

| 对比 | ArrayQueue | std::deque |
|------|------------|------------|
| 容量 | 编译期固定 | **可增长**（依赖 sbrk/malloc） |
| 实现 | 手写环形缓冲 | **标准库** — 少 bug |
| 适用 | ISR 极简 Push | 内核逻辑 **Message 队列** 等 |

**前提：** Ch9 **`sbrk`/`new`** 已通 — STL 容器 **可分配**。

→ [Ch7 ArrayQueue](../chapter-07-interrupt-fifo/notes/section-5-FIFO与ArrayQueue.md) · [Ch9 sbrk](../chapter-09-layers/notes/section-2-sbrk与new运算符.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. APIC Timer](./section-3-APIC定时器与TimerManager.md)
