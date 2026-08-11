# TLPI 第 36 章 — Process Resources

> 对应目录：`chapter-36-process-resources/`  
> 书名原文：**Process Resources**  
> ⚠️ **`getrusage` 看消耗；`getrlimit`/`setrlimit` 管上限。** 软限可在硬限内调；非特权**硬限只能降不能升**。`RUSAGE_CHILDREN` 只含**已 wait** 的子进程。

**优先级**：🔴（服务调 `NOFILE`、剖析 CPU/RSS、daemon 启动设限）  
**前置**：[Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md)  
**后置**：[Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 章节目标

`getrusage`；软/硬 `rlimit`；超限信号/错误；fork/exec 继承；与 shell `ulimit` / systemd 的关系。

---

## 36.1 `getrusage`

```c
int getrusage(int who, struct rusage *usage);
```

| who | 含义 |
|-----|------|
| `RUSAGE_SELF` | 本进程 |
| `RUSAGE_CHILDREN` | **已 wait 回收**的子进程汇总（未 wait 不计） |
| `RUSAGE_THREAD` | 当前线程（Linux） |

常用字段：`ru_utime` / `ru_stime` · `ru_maxrss`（峰值 KB，**非累加**）· `ru_minflt`/`ru_majflt` · `ru_nvcsw`/`ru_nivcsw`。

Demo：[`code/rusage_demo.c`](./code/rusage_demo.c)

---

## 36.2 `getrlimit` / `setrlimit`

```c
struct rlimit { rlim_t rlim_cur; /* soft */ rlim_t rlim_max; /* hard */ };
/* RLIM_INFINITY */
```

| 角色 | 软限 | 硬限 |
|------|------|------|
| 非特权 | 可在 `[0, hard]` 内改 | **只能降**，不能升 |
| `CAP_SYS_RESOURCE` | 任意 | 任意 |

| resource | 超限常见结果 |
|----------|----------------|
| `RLIMIT_CPU` | `SIGXCPU` → 再超 `SIGKILL` |
| `RLIMIT_FSIZE` | `SIGXFSZ` / `EFBIG` |
| `RLIMIT_DATA` / `AS` | `ENOMEM` |
| `RLIMIT_STACK` | `SIGSEGV` |
| `RLIMIT_NOFILE` | `EMFILE`（可用 fd 约到 `cur-1`） |
| `RLIMIT_NPROC` | `EAGAIN`（fork/线程） |
| `RLIMIT_CORE` | 0 = 禁止 core |
| `RLIMIT_SIGPENDING` | `sigqueue` 失败 |
| `RLIMIT_MEMLOCK` | `mlock` 失败 |

服务常调：`NOFILE`、`STACK`、`CORE`。  
Linux：`prlimit(pid, …)` 可改**他进程**（需权）。

fork / exec：**完整继承/保留**限制。  
`ulimit` 改的是 shell 再派生；**systemd unit / 程序内 `setrlimit`** 才管 daemon。

Demo：[`code/print_rlimit.c`](./code/print_rlimit.c) · [`code/raise_nofile.c`](./code/raise_nofile.c)

---

## 易错清单

1. CHILDREN 必须 wait 才进账  
2. `ru_maxrss` 是峰值  
3. 硬限降了回不去（非特权）  
4. `NOFILE` 与「最大 fd 编号」关系：大约 `cur-1`  
5. 启动时主动抬软限是常见服务套路  

---

## 实验清单

1. `getrusage` 看 CPU/RSS/切换  
2. CHILDREN + wait  
3. 打印默认 rlimit  
4. 抬高 `RLIMIT_NOFILE`（不超硬限）  
5. （选）硬限不可升  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | rusage：消耗；rlimit：上限 |
| 2 | 软 ≤ 硬；非特权硬限只降 |
| 3 | CHILDREN = 已 wait 子进程 |
| 4 | fork/exec 保留限制 |
| 5 | daemon 勿只靠交互 shell ulimit |
| 6 | 服务常调 NOFILE |

---

## 参考

- Kerrisk · TLPI Ch36  
- `man 2 getrusage` · `man 2 getrlimit` · `man 2 setrlimit` · `man 2 prlimit`
