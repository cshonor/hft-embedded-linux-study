# TLPI 第 35 章 — Process Priorities and Scheduling

> 对应目录：`chapter-35-process-priorities-scheduling/`  
> 书名原文：**Process Priorities and Scheduling**  
> ⚠️ **nice 只影响 `SCHED_OTHER`。** 实时策略 `SCHED_FIFO`/`SCHED_RR`（优先级 1–99）永远压过普通进程。实时死循环可饿死系统。

**编号：** 不是 Ch8（Ch8 = 用户与组）。后置 daemon 是 **[Ch37](../chapter-37-daemons/notes.md)**；Ch36 = 资源限制。

**优先级**：🔴（嵌入式 / HFT 调度与亲和）  
**前置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/notes.md)  
**后置**：[Ch36 进程资源](../chapter-36-process-resources/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 章节目标

nice；`SCHED_OTHER` vs FIFO/RR；`sched_*` API；权限与 `RLIMIT_RTPRIO`；CPU 亲和；`SCHED_RESET_ON_FORK`；实时风险。

---

## 35.1–35.2 Nice（`SCHED_OTHER`）

区间 **-20（更优先）…19（更低）**，默认 0；是权重，非绝对优先级。

```c
int getpriority(int which, id_t who);
int setpriority(int which, id_t who, int prio);
/* which: PRIO_PROCESS | PRIO_PGRP | PRIO_USER */
```

| 谁 | 能做什么 |
|----|----------|
| 非特权 | 通常只能把自己 nice **调大**（降权） |
| `CAP_SYS_NICE` | 任意改 |

fork 继承；exec 保持。对 FIFO/RR **无效**。

Demo：[`code/t_nice.c`](./code/t_nice.c)

---

## 35.3 三大策略

| 策略 | 静态优先级 | 要点 |
|------|------------|------|
| **SCHED_FIFO** | 1–99（越大越高） | 跑到阻塞/`yield`/被更高优先级抢；同级无时间片轮转 |
| **SCHED_RR** | 1–99 | 同 FIFO，但同级有时间片；`sched_rr_get_interval` |
| **SCHED_OTHER** | 固定 0 | nice 调权重；可被任意就绪实时进程抢占 |

**实时 (1–99) > OTHER (0)。**

---

## 35.4 调度 API

```c
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_getscheduler(pid_t pid);
int sched_setparam / sched_getparam(...);
int sched_rr_get_interval(pid_t pid, struct timespec *tp);
int sched_yield(void);   /* 勿当同步原语用 */
```

`struct sched_param { int sched_priority; };`

### `SCHED_RESET_ON_FORK`

可或进 policy：子进程 fork 后回到 `SCHED_OTHER`/优先级清零，防实时父进程繁衍占满 CPU（需特权）。

Demo：[`code/sched_view.c`](./code/sched_view.c)

---

## 35.5 CPU 亲和（Linux）

```c
sched_setaffinity / sched_getaffinity
CPU_ZERO / CPU_SET / CPU_CLR / CPU_ISSET
```

绑核减迁移与缓存失效；HFT/嵌入式常用：业务核与中断/杂务隔离。

Demo：[`code/affinity_demo.c`](./code/affinity_demo.c)

---

## 35.6 权限与限制

- 切 FIFO/RR：需 `CAP_SYS_NICE`（或等价特权）  
- 普通用户：`RLIMIT_RTPRIO`（`ulimit -r`）  
- `kernel.sched_rt_runtime_us`：默认可限实时总 CPU（防卡死）  

---

## 35.7 fork / exec

| | 策略/优先级/亲和 |
|--|------------------|
| fork | 继承（除非 `SCHED_RESET_ON_FORK`） |
| exec | **保留** |

---

## 35.8 实践陷阱（HFT / 嵌入式）

1. 实时死循环无阻塞 → 饿死系统  
2. 持锁被抢 → 优先级反转（天花板等）  
3. nice 不改实时线程  
4. 别全员 priority=99  
5. 实时线程少做阻塞 I/O  
6. `sched_yield` ≠ 锁/条件变量  

---

## 实验清单

1. nice get/set  
2. （需 root）FIFO 抢占  
3. RR interval  
4. affinity  
5. （选）`SCHED_RESET_ON_FORK`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | nice：-20…19，只管 OTHER |
| 2 | FIFO/RR：1–99，压过 OTHER |
| 3 | FIFO 无同级时间片；RR 有 |
| 4 | 实时需 CAP_SYS_NICE / RTPRIO |
| 5 | 亲和绑核减抖动 |
| 6 | RESET_ON_FORK 防子进程继承实时 |

---

## 参考

- Kerrisk · TLPI Ch35  
- `man 2 setpriority` · `man 2 sched_setscheduler` · `man 2 sched_setaffinity` · `man 7 sched`
