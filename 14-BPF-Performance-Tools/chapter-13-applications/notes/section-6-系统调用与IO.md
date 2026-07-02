# 6. 系统调用与 I/O

### `syscount`

按进程统计 **syscall 类型与次数**。

```bash
sudo syscount-bpfcc -p $(pidof my_strategy) 1
```

| 异常信号 | 可能问题 |
|----------|----------|
| 海量 **`futex`** | 锁竞争 |
| 海量 **`sched_yield`** | 错误让出 CPU /  spin 退化 |
| 海量 **`read`/`write`** | 意外同步 I/O |

→ [Ch 6 § syscount](../../chapter-06-cpus/)

### `ioprofile`

追踪 **I/O 相关 syscall**（读/写/send/recv）+ **用户态栈** — 哪段 **应用代码** 发起多余 I/O。

```bash
sudo ioprofile-bpfcc -p $(pidof myapp) 10
```

**注意：** 受 **libc 帧指针** 问题影响（见 §9）。

---
