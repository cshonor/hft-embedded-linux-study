# Ch10 Kernel Panic, Lockups, and Hangs

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

内核挂死诊断：soft lockup (CPU 长时间不调度) / hard lockup (CPU 不响应中断) / hangcheck timer / watchdog 机制 / 自定义 panic handler。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 10.1 Kernel Panic 的触发与处理 | `notes/01-panic-causes.md` |
| 10.2 Soft Lockup：CPU 长时间不调度 | `notes/02-soft-lockup.md` |
| 10.3 Hard Lockup：CPU 不响应中断 | `notes/03-hard-lockup.md` |
| 10.4 Watchdog 机制详解 | `notes/04-watchdog-mechanism.md` |
| 10.5 Hangcheck Timer | `notes/05-hangcheck-timer.md` |
| 10.6 自定义 Panic Handler | `notes/06-custom-panic-handler.md` |
| 10.7 Kdump / Kexec 崩溃转储 | `notes/07-kdump-kexec.md` |

---

## HFT 关联

精读。HFT 系统挂死时的第一诊断手段。soft lockup 通常意味着内核模块中有死循环或自旋锁持有过久。
