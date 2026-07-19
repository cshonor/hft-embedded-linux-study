# Ch 4 处理器体系结构 · Processor Architecture

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part I）

> 本章定位：**CPU 怎么执行指令** — 用教学用 **Y86-64** 从单周期 SEQ 走到五段流水线 PIPE，理解 **流水线、冒险、分支预测**。真芯片比 Y86 复杂百倍，但 **stall、bubble、branch-miss** 的直觉来自本章。  
> **提醒：** 书中 **Ch3 真机汇编** 是 **x86-64**；Y86 是仿其风格的 **简化 ISA**，不是 Linux 上另一套生产标准。→ [Ch3 §3.3](../chapter-03-machine-level-programs/notes/section-3.3-数据格式.md)

---

## 小节笔记

| 节 | 笔记 | 一句话 |
|----|------|--------|
| 4.1 Y86-64 指令集 | [section-4.1-Y86-64-ISA.md](./notes/section-4.1-Y86-64-ISA.md) | 教学用简化 ISA；寄存器/格式/寻址/算术·访存·跳转；对照真 x86-64 |
| 4.2 HCL 与组合电路 | [section-4.2-HCL逻辑与组合电路.md](./notes/section-4.2-HCL逻辑与组合电路.md) | 门、MUX、ALU、寄存器堆 — CPU 数据通路的积木 |
| 4.3 SEQ 顺序处理器 | [section-4.3-SEQ顺序处理器.md](./notes/section-4.3-SEQ顺序处理器.md) | 单周期：取指→译码→执行→访存→写回，一条指令走完再下一条 |
| 4.4 流水线原理与局限 | [section-4.4-流水线原理与局限.md](./notes/section-4.4-流水线原理与局限.md) | 多指令重叠提吞吐；延迟 vs 吞吐量；朴素流水线的瓶颈 |
| 4.5 PIPE 与冒险 | [section-4.5-PIPE流水线与冒险.md](./notes/section-4.5-PIPE流水线与冒险.md) | **重点**：数据/控制/结构冒险；转发、stall、分支冲刷 |
| 4.6 小结与模拟器 | 见 [§4.5 文末](./notes/section-4.5-PIPE流水线与冒险.md#46-小结与模拟器) | `ssim` / `psim` 等（选做） |

**学习顺序（严格从上到下）：**  
4.1 指令集 → 4.2 电路 → 4.3 SEQ → 4.4 流水线概念 → **4.5 冒险（反复看）**。

---

## 大白话 · 本章一条线

> **一条指令不是「一步做完」，而是分阶段流水作业；阶段多了，吞吐上去，但会互相踩脚。**

```
SEQ：取指 → 译码 → 执行 → 访存 → 写回   （每拍一条，简单但慢）
PIPE：五段并行填满 — 理想 CPI→1，冒险时 stall / bubble
```

### 和 HFT 的关联（低延迟根基）

1. 明白乱序、分支、内存访问为何拖慢程序  
2. 后续 cache、指令重排、少分支代码、内存屏障 — 都建立在 **流水线冒险** 直觉上  
3. 微秒级延迟：本质是少浪费时钟周期（少 stall / 少冲刷）

**§4.5 三类冒险（必吃透）：**

| 冒险 | 直觉 | HFT 味道 |
|------|------|----------|
| **数据冒险** | 指令依赖 → 停顿或等转发 | load-use、紧依赖链拉高延迟 |
| **控制冒险** | 分支跳转 → 冲刷流水线 | **分支误预测 = 延迟杀手** |
| **结构冒险** | 硬件资源冲突 | 真机多靠复制单元缓解 |

低延迟优化（无分支、拆依赖、提局部性）本质都是 **少让流水线停下来**。

**HFT 要带走的三件事（不必手画 HCL）：**

1. **分支预测失败** → 流水线清空，和 Ch3「不可预测分支」同一物理根因  
2. **数据冒险** → 真相关要停顿或转发；写代码时减少 **load-use** 距离  
3. **CPI / IPC** — `perf` 里 IPC 低，往 cache miss、分支、后端瓶颈想（→ [14-Systems-Performance Ch 6](../../15-Systems-Performance-2nd/chapter-06-cpus/)）

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
- 真实微架构：[02-Hennessy](../../03-Computer-Architecture-6th/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
