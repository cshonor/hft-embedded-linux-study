# Ch 13 应用程序 · Applications

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**把资源消耗 ↔ 应用上下文绑在一起** — Ch 6–10 从 CPU/内存/FS/网看系统；本章从 **线程、锁、syscall、USDT** 看 **哪个业务路径** 在花钱。以 **MySQL** 为主案例，方法论可迁移到 **策略进程、网关、风控服务**。
> **HFT：** **`profile` + `offcputime` + `syscount`** 是策略延迟三板斧；锁竞争看 **`pmlock`/`pmheld`**；共置 MySQL/Redis 用 **USDT/慢查询类工具** 作模板。注意 **libc 帧指针断裂** 坑。
> **上一章：** [chapter-12-languages](../chapter-12-languages/) · **下一章：** [chapter-14-kernel](../chapter-14-kernel/)

---

## 小节笔记（按原书真实小节）

| 原书小节 | 笔记 | 覆盖工具 |
|----------|------|----------|
| 13.1 背景知识（13.1.1 基础信息 / 13.1.2 MySQL 示例 / 13.1.3 BPF 能力 / 13.1.4 分析策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) | 四种线程模型、libpthread 锁、MySQL USDT 探针表、十步策略、tid 关联法 |
| 13.2.1–13.2.2 进程与线程 | [notes/section-2-BPF工具-进程与线程.md](./notes/section-2-BPF工具-进程与线程.md) | execsnoop、threadsnoop |
| 13.2.3–13.2.4 CPU 剖析 | [notes/section-3-BPF工具-CPU剖析.md](./notes/section-3-BPF工具-CPU剖析.md) | profile、threaded |
| 13.2.5–13.2.6 Off-CPU 剖析 | [notes/section-4-BPF工具-OffCPU剖析.md](./notes/section-4-BPF工具-OffCPU剖析.md) | offcputime、offcpuhist |
| 13.2.7–13.2.8 系统调用与 I/O | [notes/section-5-BPF工具-系统调用与IO.md](./notes/section-5-BPF工具-系统调用与IO.md) | syscount、ioprofile |
| 13.2.9 libc 帧指针 | [notes/section-6-BPF工具-libc帧指针.md](./notes/section-6-BPF工具-libc帧指针.md) | 栈断裂与四种修复 |
| 13.2.10–13.2.11 MySQL 专用 | [notes/section-7-BPF工具-MySQL专用.md](./notes/section-7-BPF工具-MySQL专用.md) | mysqld_qslower、mysqld_clat |
| 13.2.14 pmlock 和 pmheld | [notes/section-8-BPF工具-锁分析.md](./notes/section-8-BPF工具-锁分析.md) | pmlock、pmheld |
| 13.2.12–13.2.13, 13.2.15–16 睡眠与信号 | [notes/section-9-BPF工具-睡眠与信号.md](./notes/section-9-BPF工具-睡眠与信号.md) | naptime、signals、killsnoop、deadlock |
| 13.3–13.4 BPF 单行程序 | [notes/section-10-BPF单行程序.md](./notes/section-10-BPF单行程序.md) | BCC/bpftrace 单行 + 示例解读 |
| 13.5 小结 | [notes/section-11-小结.md](./notes/section-11-小结.md) | 主题全景表 + 方法论 |

---

## 大白话

资源层工具告诉你"哪里慢"，本章工具告诉你"**谁**在慢、**为什么**慢"：哪条 SQL 发起的 fsync、哪个函数拿着锁不放、哪个脚本里藏了个 30 秒 sleep。

## 快速导航

- 最先跑：`profile -d -p PID`（谁吃 CPU）→ `offcputime -d -p PID`（谁在等）→ `syscount -L -m -p PID`（等在什么系统调用）
- 业务上下文：USDT 优先（mysqld_qslower 模板），USDT 不可用转 uprobes（dispatch_command 模板）
- 高开销勿常驻：offcputime/offcpuhist/ioprofile/pmlock/pmheld
