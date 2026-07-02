# Ch 5 优化程序性能 · Optimizing Program Performance

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part I）

> 本章定位：**怎么让 C/C++ 跑快** — 从编译器能做什么、到循环/内存/分支微优化，再到 **循环展开、ILP、剖析找瓶颈**。全书「写代码」与 Ch4「CPU 怎么跑」的衔接章；HFT **热路径必过一遍**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 5.1–5.3 编译器能力、性能度量与示例 | [notes/section-5.1-5.3-编译器能力与性能度量.md](./notes/section-5.1-5.3-编译器能力与性能度量.md) |
| 5.4–5.6 循环、过程调用与内存引用 | [notes/section-5.4-5.6-循环过程与内存引用优化.md](./notes/section-5.4-5.6-循环过程与内存引用优化.md) |
| 5.7 理解现代处理器（5.7.1–5.7.3） | [notes/section-5.7-现代处理器抽象模型.md](./notes/section-5.7-现代处理器抽象模型.md) |
| 5.8–5.10 循环展开与提高并行性 | [notes/section-5.8-5.10-循环展开与并行优化.md](./notes/section-5.8-5.10-循环展开与并行优化.md) |
| 5.11 限制因素（溢出与分支） | [notes/section-5.11-寄存器溢出与分支预测.md](./notes/section-5.11-寄存器溢出与分支预测.md) |
| 5.12 理解内存性能 | [notes/section-5.12-加载与存储性能.md](./notes/section-5.12-加载与存储性能.md) |
| 5.13–5.15 实战技术与程序剖析 | [notes/section-5.13-5.15-剖析与实战优化.md](./notes/section-5.13-5.15-剖析与实战优化.md) |

---

## 大白话 · 本章一条线

> **先量再改；先改大头；帮编译器，别跟它对干。**

原书用 **`combine` 累加数组** 当标尺，逐级优化：

```
-O2 基线 → 去函数调用 → 少内存读写 → 展开循环 → 多累加器/重结合 → 看 IPC 与 cache
```

**HFT 铁律：**

1. **Profile 找 p（阿姆达尔）** — 别优化冷路径（→ 5.14、`perf`）
2. **Release flags 与生产一致** — `-O3 -march=native`、LTO、PGO
3. **热路径：少调用、少 load/store、少不可预测分支** — 订单簿/解码循环

---

## 本章 Checklist

- [ ] 知道编译器 **不能** 跨文件/指针别名/未知副作用 大胆优化
- [ ] 用 **CPE**（每元素周期）或 **吞吐** 表达性能，不单看总时间
- [ ] 能列举：去 `strlen` 式循环内调用、累加器局部化、`-O` 内联
- [ ] 解释 **循环展开** 如何减分支、增 ILP；多累加器打破依赖链
- [ ] 说明 **寄存器溢出**、**分支误预测** 如何吃掉收益
- [ ] 会用 `gcc -O2/-O3`、`objdump`/`perf annotate` 或 `gprof`/`perf record` 验证
- [ ] 能说出 load/store 与 **写后读 (WAR)** 对流水的影响（衔接 Ch6）

---

## HFT 精读捷径

```
5.1–5.6 + 5.14 剖析 — 日常编码与排查
5.7–5.11 — 与 Ch4 对照，理解为何这么改有效
5.12 — 与 Ch6 一起读（cache 线、load 延迟）
5.8–5.10 — 手写热循环时再抠；很多交给 `-O3`/编译器
```

---

## 相关章节

- 上一章：[../chapter-04-processor-architecture/](../chapter-04-processor-architecture/)
- 下一章：[../chapter-06-memory-hierarchy/](../chapter-06-memory-hierarchy/)
- 机器级：[../chapter-03-machine-level-programs/](../chapter-03-machine-level-programs/)
- 观测：[13-Systems-Performance Ch 13 perf](../../13-Systems-Performance-2nd/chapter-13-perf/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
