# Ch 4 BCC · BCC (BPF Compiler Collection)

> **BPF Performance Tools** · Brendan Gregg · **精读 🔴**

> 本章定位：**BCC 工具箱使用说明书 + 四大多用途工具精讲** — BPF 的**主要前端项目**，含 **70+** 开箱即用性能/排障工具。读懂本章，才能从「跑现成脚本」过渡到「用 BCC 写自己的工具」。
> **HFT：** 生产环境**内核侧粗筛与下钻**的主力载体（`runqlat`、`profile`、`tcpretrans` 等多为 BCC 实现）；理解**单用途 vs 多用途**与**内核聚合 vs 逐行打印**，避免在热路径上误用 `trace`。
> **上一章：** [chapter-03-性能分析](../chapter-03-performance-analysis/) · **下一章：** [chapter-05-bpftrace](../chapter-05-bpftrace/)

---

## 小节笔记（按原书 4.1–4.13 真实目录）

| 节 | 原书小节 | 笔记 |
|----|----------|------|
| 4.1 | BCC 的组件 | [section-1-BCC的组件.md](./notes/section-1-BCC的组件.md) |
| 4.2 | BCC 的特性 | [section-2-BCC的特性.md](./notes/section-2-BCC的特性.md) |
| 4.3 | BCC 的安装 | [section-3-BCC的安装.md](./notes/section-3-BCC的安装.md) |
| 4.4 | BCC 的工具 | [section-4-BCC的工具.md](./notes/section-4-BCC的工具.md) |
| 4.5 | funccount | [section-5-funccount.md](./notes/section-5-funccount.md) |
| 4.6 | stackcount | [section-6-stackcount.md](./notes/section-6-stackcount.md) |
| 4.7 | trace | [section-7-trace.md](./notes/section-7-trace.md) |
| 4.8 | argdist | [section-8-argdist.md](./notes/section-8-argdist.md) |
| 4.9 | 工具文档 | [section-9-工具文档.md](./notes/section-9-工具文档.md) |
| 4.10 | 开发 BCC 工具 | [section-10-开发BCC工具.md](./notes/section-10-开发BCC工具.md) |
| 4.11 | BCC 的内部实现 | [section-11-BCC的内部实现.md](./notes/section-11-BCC的内部实现.md) |
| 4.12 | BCC 的调试 | [section-12-BCC的调试.md](./notes/section-12-BCC的调试.md) |
| 4.13 | 小结 | [section-13-小结.md](./notes/section-13-小结.md) |

---

## 四大多用途工具 · 选型一图

```
事件频率高？
 ├─ 是 → "多少次"          → funccount
 │       "什么值/分布"      → argdist (-C 频率表 / -H 直方图)
 │       "哪条调用路径"     → stackcount (-f folded → 火焰图)
 └─ 否  → "每次事件的细节"  → trace (r:: 返回值、内核态过滤、%K/%U)
```

## 本章 Checklist（HFT 视角）

- [ ] **高频用聚合，低频用 trace**——`trace` 挂高频函数 = 人为性能事故，先 `funccount -i 1` 估量级。
- [ ] **排障四步固化**：funccount 估频 → argdist 看分布 → stackcount 找路径 → trace 抓细节。
- [ ] **每个工具先读 man 8**——OVERHEAD 段决定能否常驻生产；STABILITY 段决定内核升级回归范围。
- [ ] **工具名因发行版而异**——Ubuntu `-bpfcc` 后缀 / snap `bcc.` 前缀；runbook 写全路径。
- [ ] **自研选型**——复杂带参、长期维护 → BCC（约 10 倍代码量）；临时验证 → bpftrace（第 5 章）。
- [ ] **调试三板斧**——`dmesg`（内核拒绝原因）→ `bpflist -vv`（探针状态）→ `--ebpf`（最终程序源码）。
- [ ] **`bpf_trace_printk` 只上测试机**——全局共享 Ftrace 缓冲区、~1μs/次。
- [ ] **4.17+ 内核无事件残留**——崩溃自动清理 fd；老内核才需要 `reset-trace.sh`（全局核弹，慎用）。

---

## 相关章节

- 上一章：[chapter-03-性能分析](../chapter-03-performance-analysis/)
- 下一章：[chapter-05-bpftrace](../chapter-05-bpftrace/)
- 技术地基：[chapter-02-技术背景](../chapter-02-technology-background/)
- BCC 自研：[appendix-C-BCC工具开发](../appendix-C-BCC工具开发.md)
- SysPerf BPF 章：[chapter-15-bpf](../../../06.6-systems-performance/chapter-15-bpf/)
- 网络工具实践：[chapter-10-网络](../chapter-10-networking/)
