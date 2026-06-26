# Ch 13 应用程序 · Applications

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**把资源消耗 ↔ 应用上下文绑在一起** — Ch 6–10 从 CPU/内存/FS/网看系统；本章从 **线程、锁、syscall、USDT** 看 **哪个业务路径** 在花钱。以 **MySQL** 为主案例，方法论可迁移到 **策略进程、网关、风控服务**。  
> **HFT：** **`profile` + `offcputime` + `syscount`** 是策略延迟三板斧；锁竞争看 **`pmlock`/`futex`**；共置 MySQL/Redis 用 **USDT/慢查询类工具** 作模板。注意 **libc 帧指针断裂** 坑。  
> **上一章：** [chapter-12-语言.md](../chapter-12-languages/) · **下一章：** [chapter-14-内核.md](../chapter-14-kernel/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 为什么需要应用层分析 | [notes/section-1-为什么需要应用层分析.md](./notes/section-1-为什么需要应用层分析.md) |
| 2 应用程序基础 (Application Fundamenta | [notes/section-2-应用程序基础.md](./notes/section-2-应用程序基础.md) |
| 3 应用程序上下文与 USDT | [notes/section-3-应用程序上下文与USDT.md](./notes/section-3-应用程序上下文与USDT.md) |
| 4 进程与线程分析 | [notes/section-4-进程与线程分析.md](./notes/section-4-进程与线程分析.md) |
| 5 CPU 与 Off-CPU 剖析（核心） | [notes/section-5-CPU与Off-CPU剖析核心.md](./notes/section-5-CPU与Off-CPU剖析核心.md) |
| 6 系统调用与 I/O | [notes/section-6-系统调用与IO.md](./notes/section-6-系统调用与IO.md) |
| 7 MySQL 专用工具（案例） | [notes/section-7-MySQL专用工具案例.md](./notes/section-7-MySQL专用工具案例.md) |
| 8 锁与休眠排障 | [notes/section-8-锁与休眠排障.md](./notes/section-8-锁与休眠排障.md) |
| 9 信号 (Signals) | [notes/section-9-信号.md](./notes/section-9-信号.md) |
| 10 关键避坑：libc 帧指针断裂 ⚠️ | [notes/section-10-关键避坑libc帧指针断裂.md](./notes/section-10-关键避坑libc帧指针断裂.md) |
| 11 应用分析工作流 | [notes/section-11-应用分析工作流.md](./notes/section-11-应用分析工作流.md) |

---

## 大白话

> 把资源消耗 ↔ 应用上下文绑在一起

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **资源 + 应用上下文**才完整 — 「磁盘忙」要追到 **哪段代码 write**（`ioprofile`/`biostacks` Ch 9）。
- [ ] **延迟三板斧：**`profile`（在算）+ **`offcputime`（在等）** + `syscount`（在 syscall 什么）。
- [ ] **锁：**`syscount` 见 `futex` → **`pmlock`**；热路径应用层应 **无锁/细粒度**（15-HFT ch07）。
- [ ] **`naptime`**— 低 hanging fruit；策略代码禁止 sleep 轮询。
- [ ] **MySQL 工具**— 共置 DB 的模板；自研服务学 **USDT + 内核侧慢过滤**。
- [ ] **libc 帧指针**— 不解决则 Off-CPU 图 **停在 libc**；发布链 **-fno-omit-frame-pointer** 非可选。
- [ ] **短窗口、限 PID**— 与全书生产纪律一致。

---

## 相关章节

- 上一章：[chapter-12-语言.md](../chapter-12-languages/)
- 下一章：[chapter-14-内核.md](../chapter-14-kernel/)
- CPU profile/offcpu：[chapter-06-CPU.md](../chapter-06-cpus/)
- 语言/符号：[chapter-02-技术背景.md](../chapter-02-technology-background/)
- SysPerf 应用：[chapter-05-applications](../../03-Systems-Performance-2nd/chapter-05-applications/)
- HFT 无锁：[chapter-07-无锁数据结构与内存布局](../15-HFT-Low-Latency-Practice/chapter-07-无锁数据结构与内存布局.md)
- CSAPP 并发：[chapter-12-concurrent-programming](../01-CSAPP-3rd/chapter-12-concurrent-programming/)
