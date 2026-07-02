# Ch 15 BPF 技术 · BPF

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**第二版最重磅新增** — **eBPF** 把 Linux 观测从「固定工具」推进到 **可编程内核态分析**。Ch 4 把 BPF 列为工具链之一；Ch 5–14 各章反复出现的 `runqlat`、`biolatency`、`tcplife` 等 **均出自 BCC**；本章是 **BCC + bpftrace 总入口**。  
> **HFT：** 生产裸机 **bpftrace/BCC 与 perf 并列标配** — off-CPU、run queue、重传、direct reclaim；深度专书 → [14-BPF-Performance-Tools](../../14-BPF-Performance-Tools/)。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| BPF 背景与架构（15.1–15.2 基础） | [notes/section-BPF-背景与架构15.1-15.2-基础.md](./notes/section-BPF-背景与架构15.1-15.2-基础.md) |
| 15.1 BCC (BPF Compiler Collection) | [notes/section-15.1-BCC-BPF-Compiler-Collection.md](./notes/section-15.1-BCC-BPF-Compiler-Collection.md) |
| 15.2 bpftrace | [notes/section-15.2-bpftrace.md](./notes/section-15.2-bpftrace.md) |
| 15.1.7 BCC vs bpftrace | [notes/section-15.1.7-BCC-vs-bpftrace.md](./notes/section-15.1.7-BCC-vs-bpftrace.md) |

---

## 大白话 · 本章就五件事

> **eBPF = 内核里的安全小程序 + 两种输出：明细 ring buffer / 聚合 maps。**

**① 从抓包过滤器到通用内核 VM — 1992 BPF → 2013+ eBPF。**

- 程序跑在内核，**Verifier** 保证不崩内核 — 加载失败 = 改程序，别 `--force`。

**② BCC — 成套预制工具 + Python/Lua 开发框架。**

- `execsnoop`、`runqlat`、`tcplife`… — **日常直接跑**；复杂工具用 BCC 打包发布。

**③ bpftrace — 类 awk 语言，单行命令之王。**

- 即兴追 kprobe/uprobe/USDT — **ad hoc** 根因分析；本仓库 [附录 C](../appendix-C-bpftrace单行命令.md) 扩展。

**④ BCC vs bpftrace：复杂工具 vs 快速脚本 — 双剑互补。**

- 不是二选一 — **先 BCC 标准工具，不够再 bpftrace 定制**。

**⑤ Maps 在内核聚合 — 高频率事件不打爆用户态。**

- 直方图、计数器在 kernel 汇总 — 与 Ftrace hist（Ch 14）同思路，更灵活。

下面按原书 15.1–15.2 及架构基础展开。

---

## 与 perf / Ftrace 的分工（全书闭环）

| 工具 | 强项 |
|------|------|
| **perf** | CPU 采样、PMC、官方标配（Ch 13） |
| **Ftrace** | function_graph、hwlat（Ch 14） |
| **BCC** | 预制全栈工具、off-CPU、I/O/TCP 直方图 |
| **bpftrace** | 快速定制、单行 ad hoc |

```
Ch 1  60 秒清单
Ch 2  USE / 延迟分解
Ch 4  四工具链
Ch 5–10  各资源「用哪条 BCC」
Ch 13  perf
Ch 14  Ftrace
Ch 15  BPF（本章）
  → 04-BPF 专书 18 章
  → 附录 C 单行命令
```

---

## 本章 Checklist

- [ ] 裸机安装 **bcc-tools + bpftrace**，内核头匹配
- [ ] 理解 **Verifier、maps、ring buffer**
- [ ] 跑通 **runqlat、biolatency、tcpretrans** 各一次
- [ ] 写一条 **bpftrace** 统计 syscall 或 tracepoint
- [ ] 知道 **BCC 标准工具 vs bpftrace 定制** 何时切换
- [ ] 生产追踪 **限 PID、限时长** — 观测者效应（Ch 4）

---

## HFT 精读捷径（Ch 15 在路线中的位置）

```
SysPerf Ch 1–14  →  Ch 15 BPF 总入口
  → 14-BPF-Performance-Tools 全书（紧接 13 SysPerf）
  → 12-DPDK 02-Advanced XDP
  → 12-HFT 生产 runbook
```

**本章最小行动集：**

1. `sudo runqlat-bpfcc 10` — dedicated 核 run queue 延迟 baseline。
2. `sudo tcpretrans-bpfcc 30` — 发单通道有无重传。
3. `sudo bpftrace -e 'tracepoint:syscalls:sys_enter_{read,write} /pid==X/ { @[probe] = count(); }'` — 热路径 syscall 一览。
4. 将三条写入 **危机 runbook**（Ch 4）。

**Gregg 本章金句（HFT 版）：**

> **eBPF 是 Linux 观测的革命** — BCC 是 **标准武器库**，bpftrace 是 **现场即兴手术刀**。  
> **Maps 在内核聚合** — 高频率事件别往用户态灌水；**Verifier 通过** 才能上生产。

---

## 相关章节

- 上一章：[../chapter-14-ftrace/](../chapter-14-ftrace/)
- 下一章：[../chapter-16-case-studies/](../chapter-16-case-studies/)
- 工具地图：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- perf：[../chapter-13-perf/](../chapter-13-perf/)
- 应用 Off-CPU：[../chapter-05-applications/](../chapter-05-applications/)
- 附录 C：[appendix-C-bpftrace单行命令.md](../appendix-C-bpftrace单行命令.md)
- BPF 专书：[14-BPF-Performance-Tools](../../14-BPF-Performance-Tools/)
- XDP：[14-BPF note-XDP](../../14-BPF-Performance-Tools/note-XDP与tc-BPF.md) · [12-DPDK 02-Advanced](../../12-DPDK-Low-Latency-Network/02-Advanced-Book/notes/note-XDP与DPDK对照.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
