# 1. 本章要回答的两个问题

| 问题 | 工具族 | 视角 |
|------|--------|------|
| **CPU 在忙什么？** | `profile`、火焰图、`cpudist`、`syscount` | **On-CPU** — 在核上执行什么代码 |
| **线程为什么得不到 CPU？** | `runqlat`、`runqslower`、`runqlen` | **调度饱和度** — 排队多久才跑上核 |
| **线程不跑时在等什么？** | `offcputime` | **Off-CPU** — 睡眠/阻塞栈与等待时间 |

```
         On-CPU                    Off-CPU
    profile / cpudist          offcputime
    火焰图 / llcstat              |
         \                       /
          \   runqlat（就绪→上核）/
           ----------------------
              调度器 / 运行队列
```

---
