# Ch 9 磁盘 I/O · Disk I/O

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**物理块 I/O 栈** — [Ch 8](../chapter-08-file-systems/) 的逻辑 I/O 在 cache miss 或必须落盘时，下沉为对 **块设备** 的请求。磁盘/SSD/NAS 比 CPU/内存慢 **数量级**，常是系统级瓶颈；BPF 在 **低开销** 下给出 **延迟直方图、逐 I/O 明细、发起栈**。  
> **HFT：** 交易热路径 **不应触盘**；但 **日志盘打满、swap 误开、共置机后台 flush、NVMe 健康排查** 都要靠本章工具。与 [Ch 3 `biolatency`](../chapter-03-performance-analysis/) 清单衔接。  
> **上一章：** [chapter-08-文件系统.md](../chapter-08-file-systems/) · **下一章：** [chapter-10-网络.md](../chapter-10-networking/)

---

## 小节笔记（按原书 9.1–9.6 真实小节）

| 原书节 | 主题 | 笔记 |
|--------|------|------|
| 9.1 | 背景知识（块 I/O 栈 / rwbs / 调度器 / 请求时长术语 / BPF 能力 / 策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) |
| 9.2 | 传统工具（iostat / perf / blktrace / SCSI 日志） | [notes/section-2-传统工具.md](./notes/section-2-传统工具.md) |
| 9.3.1–2 | biolatency 🔴 / biosnoop | [notes/section-3-BPF工具-延迟与事件跟踪.md](./notes/section-3-BPF工具-延迟与事件跟踪.md) |
| 9.3.3–4 | biotop / bitesize | [notes/section-4-BPF工具-top与IO尺寸.md](./notes/section-4-BPF工具-top与IO尺寸.md) |
| 9.3.5–6 | seeksize / biopattern | [notes/section-5-BPF工具-模式与寻址.md](./notes/section-5-BPF工具-模式与寻址.md) |
| 9.3.7–9 | biostacks / bioerr / mdflush | [notes/section-6-BPF工具-IO栈与错误.md](./notes/section-6-BPF工具-IO栈与错误.md) |
| 9.3.10–13 | iosched / scsilatency / scsiresult / nvmelatency | [notes/section-7-BPF工具-调度器与驱动层.md](./notes/section-7-BPF工具-调度器与驱动层.md) |
| 9.4 | BPF 单行程序（BCC / bpftrace / 示例） | [notes/section-8-BPF单行程序.md](./notes/section-8-BPF单行程序.md) |
| 9.5 | 可选练习（10 题，第 10 题作者未解决） | [notes/section-9-可选练习.md](./notes/section-9-可选练习.md) |
| 9.6 | 小结 | [notes/section-10-小结.md](./notes/section-10-小结.md) |

---

## 大白话

> 物理块 I/O 栈：**先分布后个例，先拆时长再跨层归因。**

- **三板斧**：`biolatency`（延迟分布，多峰=线索）→ `biosnoop`（逐事件看排队/模式）→ `biostacks`（内核栈翻译"谁发起的"）
- **拆时长**：请求时长 = 等待（iosched / biosnoop QUE）+ 服务（设备），两段分开治
- **两分法**：issue 侧（seeksize 请求随机度）vs complete 侧（biopattern 完成随机度）；rwbs 单行一条看清负载构成

## 本章 Checklist

- [ ] **热路径零块 I/O** — 交易时段 `biotop`/`biostacks` 出现策略 PID = red flag
- [ ] **`biolatency` 是 Ch 3 清单成员** — 比 `iostat await` 更看 **长尾**；incident 10–30s 短采即可；`-F` 分同步写/flush
- [ ] **延迟尖刺查周期源** — `biostacks`（换页/newfstatat 预读栈）+ `mdflush`（filebeat 每 5s flush 类）
- [ ] **错误先量化** — `bioerr` 看 [设备,错误码] 频率；每 2s 一次 EIO 可能只是 USB 探测
- [ ] **NVMe 机器** — `nvmelatency` 分离 flush/read/write 延迟；HDD 才重点 `seeksize`
- [ ] **%util 不判饱和** — NVMe/多队列并行繁忙≠过载；await 是平均，双峰要看 biolatency

---

## 相关章节

- 上一章：[chapter-08-文件系统.md](../chapter-08-file-systems/)
- 下一章：[chapter-10-网络.md](../chapter-10-networking/)
- 内存/swap：[chapter-07-内存.md](../chapter-07-memory/)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 磁盘：[chapter-09-disks](../../../14-systems-performance/chapter-09-disks/)
- CSAPP I/O：[chapter-10-system-io](../../../02-computer-systems/chapter-10-system-io/)
