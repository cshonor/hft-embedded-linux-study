# Ch 3 性能分析 · Performance Analysis

> **BPF Performance Tools** · Brendan Gregg · 印刷 pp.71–90 · **精读 🔴**

> 本章定位：**性能分析速成课** — 不是 BPF 语法，而是**目标、四大方法论、两套检查清单**（Linux 60 秒 + BCC 11 工具）。连接 [Ch 2 技术背景](../chapter-02-technology-background/README.md) 与 [Ch 4 BCC 专章](../chapter-04-bcc/README.md)。
> **HFT：** 生产 incident 先明确目标（延迟/成本），再 **60 秒粗筛**，最后 **BCC/bpftrace 精准下钻** — 与 [SysPerf Ch 2 方法论](../../../06.6-systems-performance/chapter-02-methodologies/) 同序。
> **上一章：** [chapter-02-technology-background](../chapter-02-technology-background/README.md) · **下一章：** [chapter-04-bcc](../chapter-04-bcc/README.md)

---

## 小节笔记（按原书 3.1–3.5 真实结构）

| 节 | 原书标题 | 笔记 |
|----|----------|------|
| 3.1 | 概览（目标 / 分析工作 / 多重性能问题） | [notes/section-1-概览目标与多重性能问题.md](./notes/section-1-概览目标与多重性能问题.md) |
| 3.2 | 性能分析方法论（画像 / 下钻 / USE / 清单） | [notes/section-2-性能分析方法论.md](./notes/section-2-性能分析方法论.md) |
| 3.3 | Linux 60 秒分析（10 个传统工具逐个讲） | [notes/section-3-Linux60秒分析.md](./notes/section-3-Linux60秒分析.md) |
| 3.4 | BCC 工具检查清单（11 个工具逐个讲） | [notes/section-4-BCC工具检查清单.md](./notes/section-4-BCC工具检查清单.md) |
| 3.5 | 小结（方法论落地路径 + 坑点表） | [notes/section-5-小结.md](./notes/section-5-小结.md) |

---

## 本章 Checklist

- [ ] **先问"有已知性能问题吗"** — 无目标巡检只会产出更多数据，不是洞察。
- [ ] **60 秒 + BCC 是 runbook 骨架** — 与 SysPerf 危机工具包合并成团队一页纸。
- [ ] **直方图工具优先**（runqlat、biolatency）— 均值在 HFT 里几乎总是骗人。
- [ ] **下钻要钻到"可修的机制"** — 停在"磁盘/CPU 慢"等于没分析（时间戳案例）。
- [ ] **USE 三项全查** — 使用率 / 饱和度 / 错误；HFT 延迟问题多藏在饱和度里。
- [ ] **vmstat r 优于 load average** — load 含 D 状态（IO 等待），CPU 饱和看 r。
- [ ] **%util ≠ 容量** — 并行设备 100% 繁忙仍可能有余量，看 await/队列。
- [ ] **`profile` 找 CPU，`runqlat` 找排队，`tcpretrans` 找网** — 三条覆盖共置机 80% 内核侧嫌疑。

---

## 相关章节

- 上一章：[chapter-02-technology-background](../chapter-02-technology-background/README.md)
- 下一章：[chapter-04-bcc](../chapter-04-bcc/README.md)
- Ch 1 工具初探：[chapter-01-introduction](../chapter-01-introduction/README.md)
- SysPerf 方法论：[06.6-systems-performance Ch 2](../../../06.6-systems-performance/chapter-02-methodologies/)
- 全书目录与页码对照：[BOOK-TOC.md](../BOOK-TOC.md)
