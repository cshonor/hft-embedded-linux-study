# Ch 2 技术背景 · Technology Background

> **BPF Performance Tools** · Brendan Gregg · 印刷 pp.16–70 · **精读 🔴**

> 本章定位：**全书技术地基** — eBPF VM、Map、辅助函数、栈遍历、火焰图、四类插桩（k/u probe、Tracepoint/USDT）、动态 USDT、PMC/perf_events。后续 BCC/bpftrace 工具都建在这些组件之上。
> **HFT：** 读懂本章才能判断「这条 probe 为什么贵」「火焰图为什么缺帧」「换内核后脚本为何挂」— 避免在生产热路径上误用 per-event 输出。
> **上一章：** [chapter-01-introduction](../chapter-01-introduction/README.md) · **下一章：** [chapter-03-performance-analysis](../chapter-03-performance-analysis/README.md)

---

## 小节笔记（按原书 2.1–2.14 真实结构）

| 节 | 原书标题 | 笔记 |
|----|----------|------|
| 2.1 | 图释 BPF | [notes/section-1-图释BPF.md](./notes/section-1-图释BPF.md) |
| 2.2 | BPF（经典 BPF 历史） | [notes/section-2-BPF历史.md](./notes/section-2-BPF历史.md) |
| 2.3 | 扩展版 BPF（bpftool/API/并发/BTF/CO-RE/局限） | [notes/section-3-扩展版BPF.md](./notes/section-3-扩展版BPF.md) |
| 2.4 | 调用栈回溯（FP/DWARF/LBR/ORC/符号） | [notes/section-4-调用栈回溯.md](./notes/section-4-调用栈回溯.md) |
| 2.5 | 火焰图 | [notes/section-5-火焰图.md](./notes/section-5-火焰图.md) |
| 2.6 | 事件源 | [notes/section-6-事件源.md](./notes/section-6-事件源.md) |
| 2.7 | kprobes | [notes/section-7-kprobes.md](./notes/section-7-kprobes.md) |
| 2.8 | uprobes | [notes/section-8-uprobes.md](./notes/section-8-uprobes.md) |
| 2.9 | 跟踪点 tracepoints | [notes/section-9-跟踪点tracepoints.md](./notes/section-9-跟踪点tracepoints.md) |
| 2.10 | USDT | [notes/section-10-USDT.md](./notes/section-10-USDT.md) |
| 2.11 | 动态 USDT | [notes/section-11-动态USDT.md](./notes/section-11-动态USDT.md) |
| 2.12 | 性能监控计数器 PMC | [notes/section-12-性能监控计数器PMC.md](./notes/section-12-性能监控计数器PMC.md) |
| 2.13 | perf_events | [notes/section-13-perf_events.md](./notes/section-13-perf_events.md) |
| 2.14 | 小结（全章技术地图 + 坑点表） | [notes/section-14-小结与全章技术地图.md](./notes/section-14-小结与全章技术地图.md) |

---

## 本章 Checklist

- [ ] **内核聚合、用户展示** — 热路径上只 export 统计（map 直方图），不 export 原始事件流。
- [ ] **先静态后动态** — tracepoint/USDT 优先，kprobe/uprobe 兜底；内核升级维护成本差一个数量级。
- [ ] **uprobe 远离高频函数** — malloc/free 每秒百万级，uprobe 可致 10 倍损耗；改用低频事件或 USDT。
- [ ] **火焰图要栈得先要有帧** — 关键服务 `-fno-omit-frame-pointer` 或配 debuginfo，否则满屏 [unknown]。
- [ ] **并发计数用 per-CPU map** — 高频探针上普通 hash map 必然丢失更新。
- [ ] **7×24 常挂用 BPF_RAW_TRACEPOINT**（4.17+，压测近基线性能）。
- [ ] **CO-RE 是跨内核部署的未来** — 自研工具长期规划 libbpf + BTF（`/sys/kernel/btf/vmlinux`）。
- [ ] **虚机先验 PMC** — `perf stat` 全 0 说明云环境没暴露硬件计数器。

---

## 相关章节

- 上一章：[chapter-01-introduction](../chapter-01-introduction/README.md)
- 下一章：[chapter-03-performance-analysis](../chapter-03-performance-analysis/README.md)
- BCC：[chapter-04-bcc](../chapter-04-bcc/README.md) · bpftrace：[chapter-05-bpftrace](../chapter-05-bpftrace/README.md)
- CPU / PMC 实践：[chapter-06-cpus](../chapter-06-cpus/README.md)
- C / CO-RE：[appendix-D-C语言BPF.md](../appendix-D-C语言BPF.md)
- 全书目录与页码对照：[BOOK-TOC.md](../BOOK-TOC.md)
