# 第 26 章 · Garbage Collection（垃圾回收） · 标记阶段与三色抽象（Marking & Tri-color Abstraction）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-sweeping.md](./02-sweeping.md)

---

**根 (Roots)**：GC 起点 — 凡 **mutator 直接可达** 之处：

| 根 | 示例 |
|----|------|
| **栈** | `vm.stack` 上所有 Value |
| **全局** | **`vm.globals`** |
| **CallFrame** | closure / function 引用 |
| **Upvalue** | open/closed 链 |
| **编译期常量** | 当前 chunk 常量池（实现随版本） |

**三色**（概念模型）：

| 色 | 含义 |
|----|------|
| **白** | 未访问 · sweep 时若仍白 → **垃圾** |
| **灰** | 已发现 · **引用尚未全部扫描** · 在 **工作栈/列表** |
| **黑** | 自身及 **直接引用对象** 均已标记 |

**算法（DFS / 工作栈）**：

```text
markRoots() → 所有根标灰并入 work
while work not empty:
  obj = pop(work)
  mark 其引用的 Obj（子对象）
  obj → 黑
```

**循环引用**：对象图 **有环** 时，仅从根 DFS + **visited/mark 位** 避免死循环；不可达环整体仍白 → 被回收。

**实现**：`Obj` 上 **`isMarked` 位**（或深色标记）；`markObject` / `markValue` / `markTable` 递归。

**对照 RFR / 系统**：Rust **所有权** 编译期证明；clox **运行时 tracing GC** — 与 JVM、Go 同族。

---
