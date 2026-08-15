# 第 6 章 · 过程抽象 · §6 内存管理与 GC

← [本章目录](./README.md) · 上一节：[05-call-linkages.md](./05-call-linkages.md) · 下一节：---

除**栈上 AR** 外，编译器管理整个**逻辑地址空间**：

```text
┌ 代码区 (text) ─────────────────┐
├ 静态/全局数据 (data/bss) ──────┤
├ 堆 (heap) ── 动态、生命周期不定 ┤
├        ↓ 增长方向               │
├        ↑ 增长方向               │
└ 栈 (stack) ── AR、局部 ─────────┘
```

→ RFR 第 1 章 · [03-2 layout](../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md)

---

## 堆管理（Heap Management）

**生命周期不可预测**的分配：

| 来源 | 例子 |
|------|------|
| **`malloc` / `new`** | C / C++ / Java 对象 |
| **闭包环境** | 捕获变量堆分配（clox ObjUpvalue） |

### 显式分配算法（教学）

| 算法 | 思路 |
|------|------|
| **First-fit** | 找第一个够大的空闲块 |
| **Best-fit / 其他** | 减少碎片 — 工程更复杂 |

**Rust**：`GlobalAlloc` trait；默认 allocator；**无 GC** — 所有权 + `Drop`。

---

## 隐式释放：垃圾回收（GC）

语言**不支持**手动 `free` 时 — **GC** 回收不可达对象。

| 算法 | 要点 |
|------|------|
| **引用计数（Reference counting）** | 简单；循环引用需弱引用等 |
| **标记-清扫（Mark-sweep）** | 从根集合标记可达；清扫未标记 |
| **拷贝回收（Copying / Cheney）** | 半空间复制；整理碎片 |

**jlox**：借 **Java GC**  
**clox**：**ch26 自实现 Mark-sweep** → [GC 笔记](../../../01_Crafting-Interpreters/part03_clox/chapter26_garbage-collection/README.md)

---

## 栈 vs 堆（编译器视角）

| | **栈（AR）** | **堆** |
|---|-------------|--------|
| 分配 | 调用/返回自动 | `malloc`/GC |
| 速度 | **快** | 较慢 |
| 生命周期 | 调用边界 | 任意 |
| **HFT** | 热路径优先栈/寄存器 | 避免分配抖动 |

**`no_std`**：可能无默认堆 — 静态/栈-only 子集。

---

## 本章收束

过程抽象 = **控制（call/return）** + **名字（scope）** + **空间（AR/堆）** + **契约（ABI）** + **回收（GC/手动）**。

→ 下一章 **ch7 代码形态** — 表达式/控制流如何 lowering 到 IR。
