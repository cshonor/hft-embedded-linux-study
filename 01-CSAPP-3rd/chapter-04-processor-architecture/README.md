# Ch 4 处理器体系结构 · Processor Architecture

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part I）

> 本章定位：**CPU 怎么执行指令** — 用教学用 **Y86-64** 从单周期 SEQ 走到五段流水线 PIPE，理解 **流水线、冒险、分支预测**。真芯片比 Y86 复杂百倍，但 **stall、bubble、branch-miss** 的直觉来自本章。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 4.1 Y86-64 指令集（4.1.1–4.1.6） | [notes/section-4.1-Y86-64-ISA.md](./notes/section-4.1-Y86-64-ISA.md) |
| 4.2 HCL 逻辑设计（4.2.1–4.2.5） | [notes/section-4.2-HCL逻辑与组合电路.md](./notes/section-4.2-HCL逻辑与组合电路.md) |
| 4.3 SEQ 顺序实现（4.3.1–4.3.4） | [notes/section-4.3-SEQ顺序处理器.md](./notes/section-4.3-SEQ顺序处理器.md) |
| 4.4 流水线通用原理（4.4.1–4.4.4） | [notes/section-4.4-流水线原理与局限.md](./notes/section-4.4-流水线原理与局限.md) |
| 4.5 流水线 PIPE（4.5.1–4.5.10） | [notes/section-4.5-PIPE流水线与冒险.md](./notes/section-4.5-PIPE流水线与冒险.md) |
| 4.6 小结与模拟器 | 见 [section-4.5](./notes/section-4.5-PIPE流水线与冒险.md#46-小结与模拟器) |

---

## 大白话 · 本章一条线

> **一条指令不是「一步做完」，而是分阶段流水作业；阶段多了，吞吐上去，但会互相踩脚。**

```
SEQ：取指 → 译码 → 执行 → 访存 → 写回   （每拍一条，简单但慢）
PIPE：五段并行填满 — 理想 CPI→1，冒险时 stall / bubble
```

**HFT 要带走的三件事（不必手画 HCL）：**

1. **分支预测失败** → 流水线清空，和 Ch3「不可预测分支」同一物理根因
2. **数据冒险** → 真相关要停顿或转发；写代码时减少 **load-use** 距离
3. **CPI / IPC** — `perf` 里 IPC 低，往 cache miss、分支、后端瓶颈想（→ [13-Systems-Performance Ch 6](../../13-Systems-Performance-2nd/chapter-06-cpus/)）

---

## 本章 Checklist

- [ ] 说出 Y86-64 程序员可见状态：PC、寄存器文件、CC、Stat
- [ ] 区分 **组合逻辑** vs **时序逻辑**（寄存器 + 时钟）
- [ ] 画出 SEQ 五阶段数据通路（文字级即可）
- [ ] 解释 **吞吐 vs 延迟**；理想流水线加速比上界
- [ ] 分类冒险：**结构 / 数据 / 控制**；各自典型对策
- [ ] 说明 **转发 (forwarding)** 解决哪些 RAW；何时仍须 stall
- [ ] 知道 `csim`/`ssim`/`psim` 是本章配套模拟器（选做）

---

## HFT 精读捷径

```
理论必读：4.4 流水线局限 + 4.5.5 冒险 + 4.5.9 性能
与优化衔接：4.4 → Ch 5（循环展开、分支、ILP）
与观测衔接：branch-misses、cycles、IPC → SysPerf Ch 6/13
Y86/HCL/SEQ 细节：作业或第一遍扫读；复习抓 PIPE 冒险表
4.2 HCL：读懂即可，不必默写
```

---

## 相关章节

- 上一章：[../chapter-03-machine-level-programs/](../chapter-03-machine-level-programs/)
- 下一章：[../chapter-05-optimizing-performance/](../chapter-05-optimizing-performance/)
- 真实微架构：[02-Hennessy](../../02-Computer-Architecture-6th/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
