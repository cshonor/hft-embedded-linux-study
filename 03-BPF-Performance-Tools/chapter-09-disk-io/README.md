# Ch 9 磁盘 I/O · Disk I/O

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**物理块 I/O 栈** — [Ch 8](../chapter-08-file-systems/) 的逻辑 I/O 在 cache miss 或必须落盘时，下沉为对 **块设备** 的请求。磁盘/SSD/NAS 比 CPU/内存慢 **数量级**，常是系统级瓶颈；BPF 在 **低开销** 下给出 **延迟直方图、逐 I/O 明细、发起栈**。  
> **HFT：** 交易热路径 **不应触盘**；本章 **⚪ 默认跳过**，用于 **日志盘打满、swap 误开、共置机后台 flush、NVMe 健康排查** 等。与 [Ch 3 `biolatency`](../chapter-03-performance-analysis/) 清单衔接。  
> **上一章：** [chapter-08-文件系统.md](../chapter-08-file-systems/) · **下一章：** [chapter-10-网络.md](../chapter-10-networking/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章要回答的问题 | [notes/section-1-本章要回答的问题.md](./notes/section-1-本章要回答的问题.md) |
| 2 磁盘 I/O 基础与块 I/O 栈 | [notes/section-2-磁盘IO基础与块IO栈.md](./notes/section-2-磁盘IO基础与块IO栈.md) |
| 3 传统磁盘分析工具 | [notes/section-3-传统磁盘分析工具.md](./notes/section-3-传统磁盘分析工具.md) |
| 4 基础延迟与实时观测 | [notes/section-4-基础延迟与实时观测.md](./notes/section-4-基础延迟与实时观测.md) |
| 5 I/O 模式与尺寸特征 | [notes/section-5-IO模式与尺寸特征.md](./notes/section-5-IO模式与尺寸特征.md) |
| 6 深层排障：栈、错误与驱动层 | [notes/section-6-深层排障栈错误与驱动层.md](./notes/section-6-深层排障栈错误与驱动层.md) |
| 7 工具选型速查 | [notes/section-7-工具选型速查.md](./notes/section-7-工具选型速查.md) |
| 8 与 Ch 8 / Ch 7 的下钻链 | [notes/section-8-与Ch8Ch7的下钻链.md](./notes/section-8-与Ch8Ch7的下钻链.md) |
| 9 BPF / bpftrace One-Liners（示意） | [notes/section-9-BPFbpftraceOne-Liners示意.md](./notes/section-9-BPFbpftraceOne-Liners示意.md) |

---

## 大白话

> 物理块 I/O 栈

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **热路径零块 I/O**— 若 `biotop`/`biostacks` 在交易时段有策略 PID，即严重 red flag。
- [ ] **`biolatency` 是 Ch 3 清单成员**— 比 `iostat await` 更看 **长尾**；incident 10–30s 短采即可。
- [ ] **`biostacks` + swap 栈**— 延迟尖刺且 CPU/网正常时，查 **swap 是否误开**（[Ch 7 `swapin`](./chapter-07-内存.md)）。
- [ ] **共置机日志盘**— `biotop` 找进程 → `biostacks` 找 `journal`/`writeback` 栈。
- [ ] **NVMe 机器**用 `nvmelatency` 分离设备 vs OS；HDD 场景才重点 `seeksize`。

---

## 相关章节

- 上一章：[chapter-08-文件系统.md](../chapter-08-file-systems/)
- 下一章：[chapter-10-网络.md](../chapter-10-networking/)
- 内存/swap：[chapter-07-内存.md](../chapter-07-memory/)
- 检查清单：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
- SysPerf 磁盘：[chapter-09-disks](../../02-Systems-Performance-2nd/chapter-09-disks/)
- CSAPP I/O：[chapter-10-system-io](../01-CSAPP-3rd/chapter-10-system-io/)
