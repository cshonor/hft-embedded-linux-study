## ① 与内核通信 · Communicating with the Kernel

**系统调用** = 用户空间访问内核服务的 **主要合法入口**（另：**异常 / 陷入** 也可进内核，但语义不同）。

```
应用程序
    │  POSIX API（read/write/open…）
    ▼
  libc（glibc 等）── 包装 ──► 真正 syscall
    │
    ▼
  内核 sys_* 实现
```

| 层次 | 谁 |
|------|-----|
| **用户写的** | `printf` → 往往经 libc，底层或 `write` |
| **实际跨界** | **`syscall` 指令 / 软中断** 等 |
| **内核** | `sys_read()` 等 |

#### Unix 设计原则

> **提供机制，而不是策略**（mechanism, not policy）

| 机制 | 策略 |
|------|------|
| 内核提供 **抽象能力**（读 fd、映射内存） | **用户程序决定** 何时读、读多少、怎么用 |

**HFT：** 热路径倾向 **批量 I/O、`mmap`、用户态轮询/DPDK** — 本质是在 **减少机制调用次数**。

→ [03 SysPerf §3.2 syscall 成本](../../../../13-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [Ch 1 user/kernel 边界](../../chapter-01-intro/)

---
