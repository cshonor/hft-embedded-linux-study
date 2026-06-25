# Ch 3 性能分析 · Performance Analysis

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**性能分析速成课** — 不是 BPF 语法，而是 **目标、方法论、两套检查清单**（Linux 60 秒 + BCC 精选）。连接 [Ch 2 技术背景](../chapter-02-technology-background/) 与 [Ch 4 BCC 专章](../chapter-04-bcc/)。  
> **HFT：** 生产 incident 先 **业务指标 / 应用 span** 锁定嫌疑，再 **60 秒粗筛**，最后 **BCC/bpftrace 精准下钻** — 与 [SysPerf Ch 2 方法论](../../02-Systems-Performance-2nd/chapter-02-methodologies/) 同序。  
> **上一章：** [chapter-02-技术背景.md](../chapter-02-technology-background/) · **下一章：** [chapter-04-BCC.md](../chapter-04-bcc/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 性能分析的概述与目标 | [notes/section-1-性能分析的概述与目标.md](./notes/section-1-性能分析的概述与目标.md) |
| 2 核心方法论 | [notes/section-2-核心方法论.md](./notes/section-2-核心方法论.md) |
| 3 Linux 60 秒检查清单（传统工具） | [notes/section-3-Linux60秒检查清单传统工具.md](./notes/section-3-Linux60秒检查清单传统工具.md) |
| 4 BCC 工具检查清单（BPF 深探） | [notes/section-4-BCC工具检查清单BPF深探.md](./notes/section-4-BCC工具检查清单BPF深探.md) |
| 5 两套清单如何串联 | [notes/section-5-两套清单如何串联.md](./notes/section-5-两套清单如何串联.md) |

---

## 大白话

> 性能分析速成课

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **Ch 3 不是可选哲学**— 没有方法论，BPF 只会产出 **更多数据**。
- [ ] **60 秒 + BCC**是 runbook 骨架 — 与 SysPerf **危机工具包** 合并成团队一页纸。
- [ ] **直方图工具优先**（`runqlat`、`biolatency`）— 均值在 HFT 里几乎总是骗人。
- [ ] **`profile` 找 CPU，`runqlat` 找排队，`tcpretrans` 找网**— 三条覆盖共置机 80% 内核侧嫌疑。

---

## 相关章节

- 上一章：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- 下一章：[chapter-04-BCC.md](../chapter-04-bcc/)
- Ch 1 工具初探：[chapter-01-简介.md](../chapter-01-introduction/)
- SysPerf 方法论：[02-SysPerf Ch 2](../../02-Systems-Performance-2nd/chapter-02-methodologies/)
- 附录 A bpftrace 单行：[appendix-A-bpftrace单行命令.md](../appendix-A-bpftrace单行命令.md)
