# Ch 4 观测工具 · Observability Tools

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**观测工具地图** — 不只教「跑哪条命令」，更讲清 **数据从哪来、工具有何原理、开销多大、能否信**。Ch 2 的 USE/延迟分解需要工具落地；Ch 3 的内核概念是理解数据源的前提；本章选工具；Ch 13–15 深入 perf/Ftrace/BPF。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 4.1 工具覆盖范围与「危机工具」 | [notes/section-4.1-工具覆盖范围与危机工具.md](./notes/section-4.1-工具覆盖范围与危机工具.md) |
| 4.2 工具的分类与原理 | [notes/section-4.2-工具的分类与原理.md](./notes/section-4.2-工具的分类与原理.md) |
| 4.3 核心观测数据源 | [notes/section-4.3-核心观测数据源.md](./notes/section-4.3-核心观测数据源.md) |
| 4.4 sar 工具 | [notes/section-4.4-sar-工具.md](./notes/section-4.4-sar-工具.md) |
| 4.5 四大追踪器 | [notes/section-4.5-四大追踪器.md](./notes/section-4.5-四大追踪器.md) |
| 4.6 观测的观测（Observing Observability） | [notes/section-4.6-观测的观测Observing-Observability.md](./notes/section-4.6-观测的观测Observing-Observability.md) |

---

## 大白话 · 本章就五件事

> 工具会换，分类和数据源不会。

**① 危机来了再装工具 — 往往已经晚了。**

- 生产出问题时临时 `apt install perf` = 拖长故障。
- **提前**在镜像/裸机装好：`procps`、`sysstat`、`linux-tools-common`（perf）、`bcc-tools`、`bpftrace`。

**② 工具按两个维度选：看谁 + 怎么看。**

| 维度 | 选项 |
|------|------|
| **看谁** | **系统级**（整机） vs **进程级**（某个 PID） |
| **怎么看** | 计数器 → 剖析 → 追踪 → 监控（细节↑ 开销↑） |

**③ 数据从内核/硬件「接口」来 — 不是工具魔法。**

- 传统：`/proc`、`/sys`
- 硬件：**PMC**（周期、cache miss）
- 静态探针：**tracepoint**（内核）、**USDT**（用户态库/应用）
- 动态探针：**kprobe/uprobe**（强大但不稳定，最后手段）

**④ 四条工具链分工：perf / Ftrace / BCC / bpftrace。**

- **perf**：CPU 剖析、PMC、部分 trace
- **Ftrace**：内核路径、调度、irq
- **BCC / bpftrace**：eBPF 可编程全栈 — HFT 长期主战场

**⑤ 别盲信数字 — 工具也会错，观测本身有成本。**

- 手册错、内核指标 bug、**观测者效应**（tracing _every_ syscall 会把系统打慢）— 都要批判性看待。

下面按原书 4.1–4.6 展开。

---

## 工具选型 · 对照 Ch 2 方法论

| Ch 2 方法 | 首选工具层 |
|-----------|------------|
| **USE（资源）** | vmstat, mpstat, sar, iostat, `/proc/net/dev` |
| **RED（服务）** | 自研 metrics + pidstat；网关可用 Prometheus |
| **延迟分解** | USDT/日志时间戳、bpftrace tracepoint、perf sched |
| **CPU 热点** | perf record → 火焰图 |
| **历史波动** | sar -f / sadc |
| **深度内核** | Ftrace / bpftrace |

---

## 危机响应 · 推荐顺序（HFT）

```
0. 工具已预装（4.1）
1. Ch 1 60 秒清单（uptime, vmstat, mpstat, pidstat, sar -n DEV…）
2. 定位 PID/核 → pidstat -t -p PID 1
3. CPU 高 → perf top / 短 perf record
4. 延迟尖刺 → bpftrace 一行（runqlat、syscall 计数）— 限 30s
5. 网络 → sar -n EDEV；必要时短 tcpdump
6. 需内核路径 → Ftrace / bpftrace tracepoint
7. 归档 sar + 记录工具版本，供事后「观测的观测」
```

---

## 本章学习目标 · 自检

- [ ] 能列出 **危机工具包** 应预装的包名
- [ ] 能区分 **固定计数器 / 剖析 / 追踪 / 监控** 的开销与细节
- [ ] 能区分 **系统级 vs 进程级** 工具选型
- [ ] 说清 **/proc、PMC、tracepoint、USDT、kprobe、uprobe** 各是什么
- [ ] 知道 **sar + sadc** 对历史分析的价值
- [ ] 能说明 **perf / Ftrace / BCC / bpftrace** 分工
- [ ] 理解 **观测者效应**，生产追踪会限时长

---

## HFT 精读捷径（Ch 4 在路线中的位置）

```
Ch 1  60 秒清单
Ch 2  USE / 延迟分解
Ch 3  内核与 syscall（理解数据源）
Ch 4  观测工具（本章：选型 + 数据源）
  → Ch 6/7/10 资源专章（带着工具读）
  → Ch 13 perf
  → Ch 14 Ftrace
  → Ch 15 BPF + 附录 C + 04-BPF
```

**本章最小行动集：**

1. 在 dev/裸机装好 **perf + bpftrace**，跑通 `perf stat` 与一条附录 C 脚本。
2. 配置 **sadc** 归档，练习 `sar -f` 读昨天 CPU/网络。
3. 对策略进程做一次 **99 Hz perf record → 火焰图**，对照 Ch 2 延迟分解。

---

## 相关章节

- 上一章：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- 下一章：[../chapter-05-applications/](../chapter-05-applications/)
- perf：[../chapter-13-perf/](../chapter-13-perf/)
- Ftrace：[../chapter-14-ftrace/](../chapter-14-ftrace/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- 附录 A USE：[appendix-A-USE方法Linux.md](../appendix-A-USE方法Linux.md)
- 附录 B sar：[appendix-B-sar总结.md](../appendix-B-sar总结.md)
- 附录 C bpftrace：[appendix-C-bpftrace单行命令.md](../appendix-C-bpftrace单行命令.md)
- BPF 专书：[14-BPF-Performance-Tools](../../14-BPF-Performance-Tools/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
