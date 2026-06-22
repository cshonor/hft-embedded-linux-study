# Ch 14 Ftrace 跟踪 · Ftrace

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**Linux 内核内置的标准追踪框架** — 无需额外安装，通过 **tracefs** 配置，适合 **探索内核路径、调度、irq、函数调用图**。Ch 13 perf 可挂 tracepoint；Ch 15 BPF 是现代 HFT **主战场**；Ftrace 在 **内核 odd case、hwlat/SMI、无 BPF 老内核** 仍不可替代。  
> **HFT：** 日常用 **perf + bpftrace**；遇 **无法解释的停顿**（非 CPU/非 I/O/非锁）→ **hwlat**；深度查 **内核收发包/调度链** → `function_graph` / trace-cmd。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 14.1–14.2 核心能力与 tracefs | [notes/section-14.1-14.2-核心能力与-tracefs.md](./notes/section-14.1-14.2-核心能力与-tracefs.md) |
| 14.3–14.4、14.8 函数追踪与 function_graph | [notes/section-14.3-14.414.8-函数追踪与-function_graph.md](./notes/section-14.3-14.414.8-函数追踪与-function_graph.md) |
| 14.5–14.7、14.10 事件源、Filter 与 Hist Trigge | [notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md](./notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md) |
| 14.9 硬件延迟检测（hwlat） | [notes/section-14.9-硬件延迟检测hwlat.md](./notes/section-14.9-硬件延迟检测hwlat.md) |
| 14.11–14.13 前端工具 | [notes/section-14.11-14.13-前端工具.md](./notes/section-14.11-14.13-前端工具.md) |

---

## 大白话 · 本章就五件事

> **Ftrace = 内核自带的「黑匣子」，写文件就能追。**

**① tracefs — 通过 `/sys/kernel/tracing` 读写配置追踪。**

- 内核自带，嵌入式/救援环境也能用 — 不依赖 BCC 包。

**② 函数追踪 + function_graph — 看内核谁调谁、耗多久。**

- 不只函数入口 — **入口+出口** 画 **带耗时的调用图**。

**③ 多事件源：tracepoint、kprobe、uprobe + filter/trigger。**

- **Hist trigger** — 内核里直接做直方图，聚合开销低。

**④ hwlat — 抓硬件/固件级停顿（含 SMI）。**

- P99 尖刺在 perf/BPF 都对不上时 — 怀疑 **BIOS/SMI**。

**⑤ 前端：trace-cmd、KernelShark、perf ftrace、Gregg perf-tools。**

- 老内核无 BPF → **perf-tools** 里 Ftrace 版 opensnoop/execsnoop 仍好用。

下面按原书 14.1–14.13 展开。

---

## Ftrace vs perf vs BPF（选型）

| 需求 | 首选 |
|------|------|
| CPU 火焰图、PMC | **perf record**（Ch 13） |
| 内核函数 **调用图+耗时** | **Ftrace function_graph** |
| 高率事件 **内核直方图** | Ftrace hist / **BPF maps** |
| 硬件 **SMI 级停顿** | **Ftrace hwlat** |
| 生产 **通用追踪** | **bpftrace/BCC**（Ch 15） |
| 无 BPF 老系统 | **perf-tools / trace-cmd** |

```
Ch 4 工具地图
  perf ──────── 计数/采样/部分 trace
  Ftrace ────── 内核路径/graph/hwlat/hist
  BPF ───────── 可编程、生产主力（下一章）
```

---

## 本章 Checklist

- [ ] 知道 **tracefs** 路径与 `current_tracer` / `tracing_on`
- [ ] 理解 **function_graph** 与裸 **function** tracer 的区别
- [ ] 启用过至少一个 **tracepoint**（如 sched_switch）
- [ ] 知道 **hwlat** 用于 SMI/固件停顿排查
- [ ] 会用 **trace-cmd record** 保存 trace
- [ ] 明确 **HFT 日常 perf/BPF，Ftrace 补内核深度与 hwlat**

---

## HFT 精读捷径（Ch 14 在路线中的位置）

```
Ch 13  perf — 默认剖析器
Ch 14  Ftrace（本章：内核内置、function_graph、hwlat）
Ch 15  BPF — 生产追踪主力
  → 09 Rosen 内核网络路径
  → 10 网络 Ch10 softirq/NAPI
  → 03-BPF 专书
```

**按需精读：**

| 场景 | 读哪节 |
|------|--------|
| 内核收发包慢 | 14.3–14.4 function_graph + net tracepoint |
| 调度/jitter | 14.5 sched tracepoint + hist |
| 莫名尖刺 | **14.9 hwlat** |
| 老内核无 BPF | 14.11–14.13 perf-tools |

**本章最小行动集：**

1. 确认 **`/sys/kernel/tracing`** 可访问。
2. **`trace-cmd record -e sched sleep 2`** + `trace-cmd report | head`。
3. 低负载跑 **30s hwlat** — 记录是否有异常 latency 事件（baseline）。

**Gregg 本章金句（HFT 版）：**

> **Ftrace 是内核自带的显微镜** — 写 tracefs 就能追；**function_graph** 看内核链，**hwlat** 看固件鬼影。  
> 新系统 **BPF 优先**；Ftrace 在 **graph、hist、hwlat** 和 **无 BPF 环境** 仍不可替代。

---

## 相关章节

- 上一章：[../chapter-13-perf/](../chapter-13-perf/)
- 下一章：[../chapter-15-bpf/](../chapter-15-bpf/)
- 工具地图：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- 内核网络：[09-Linux-Kernel-Networking](../../09-Linux-Kernel-Networking/)
- BPF 专书：[03-BPF-Performance-Tools](../../03-BPF-Performance-Tools/)
- HFT 调优：[11-HFT ch05](../../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
