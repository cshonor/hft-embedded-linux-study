# Ch10 Kernel Panic, Lockups, and Hangs

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

内核挂死诊断：soft lockup (CPU 长时间不调度) / hard lockup (CPU 不响应中断) / hangcheck timer / watchdog 机制 / 自定义 panic handler。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 10.1 Kernel Panic 的触发与处理 | `notes/section-10-1.md` |
| 10.2 Soft Lockup：CPU 长时间不调度 | `notes/section-10-2.md` |
| 10.3 Hard Lockup：CPU 不响应中断 | `notes/section-10-3.md` |
| 10.4 Watchdog 机制详解 | `notes/section-10-4.md` |
| 10.5 Hangcheck Timer | `notes/section-10-5.md` |
| 10.6 自定义 Panic Handler | `notes/section-10-6.md` |
| 10.7 Kdump / Kexec 崩溃转储 | `notes/section-10-7.md` |

---

## HFT 关联

精读。HFT 系统挂死时的第一诊断手段。soft lockup 通常意味着内核模块中有死循环或自旋锁持有过久。
