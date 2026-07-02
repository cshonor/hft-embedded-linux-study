## ⑦ 与调度相关的系统调用

| 系统调用 | 作用 |
|----------|------|
| **`nice()`** | 设置 **nice** — 改 CFS 权重 |
| **`sched_setscheduler()`** | 改 **调度策略**（`SCHED_OTHER` / `FIFO` / `RR` …）及 RT 优先级 |
| **`sched_setaffinity()`** | **CPU 亲和性** — 限制在哪些核上跑 |
| **`sched_yield()`** | **主动让出** CPU — 同优先级重新排队 |

#### 用户态常用包装

```bash
nice -n 10 ./worker          # 降低 CFS 权重
taskset -c 2,3 ./gateway     # 绑核 2、3
chrt -f 80 ./hot_thread      # SCHED_FIFO 优先级 80
```

| 工具 | 对应 |
|------|------|
| **`taskset`** | `sched_setaffinity` |
| **`chrt`** | `sched_setscheduler` |

→ [03 SysPerf §6.9 CPU 调优](../../../../13-Systems-Performance-2nd/chapter-06-cpus/notes/section-6.9-CPU-调优.md)

---
