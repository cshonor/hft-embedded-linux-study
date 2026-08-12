# TLPI 第 35 章 — Process Priorities and Scheduling

**优先级**：🔴（嵌入式 / HFT 调度与亲和）  
**前置**：[Ch34 进程组/会话](../chapter-34-process-groups-sessions/notes.md)  
**后置**：[Ch36 进程资源](../chapter-36-process-resources/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 小节目录

- [35.1 –35.2 Nice（`SCHED_OTHER`）](./notes/35.1-schedother.md)
- [35.3 三大策略](./notes/35.3-strategy.md)
- [35.4 调度 API](./notes/35.4-api.md)
- [35.5 CPU 亲和（Linux）](./notes/35.5-cpu.md)
- [35.6 权限与限制](./notes/35.6-permission-limits.md)
- [35.7 fork / exec](./notes/35.7-fork-exec.md)
- [35.8 实践陷阱（HFT / 嵌入式）](./notes/35.8-hft.md)

---

## 章节目标


nice；`SCHED_OTHER` vs FIFO/RR；`sched_*` API；权限与 `RLIMIT_RTPRIO`；CPU 亲和；`SCHED_RESET_ON_FORK`；实时风险。

---


---

## 实验清单


1. nice get/set  
2. （需 root）FIFO 抢占  
3. RR interval  
4. affinity  
5. （选）`SCHED_RESET_ON_FORK`  

---


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


---

## 参考


- Kerrisk · TLPI Ch35  
- `man 2 setpriority` · `man 2 sched_setscheduler` · `man 2 sched_setaffinity` · `man 7 sched`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>

/* Ch35 进程优先级与调度 — nice/getpriority/setpriority/sched。
 * 演示 nice 值调整 + 实时调度策略。
 * 编译: gcc -o ch35_demo ch35_demo.c */

int main(void) {
    /* nice 值: -20 (最高优先级) 到 +19 (最低优先级) */
    int nice_val = nice(0);  /* 0 = 查询当前值 */
    printf("Current nice value: %d\n", nice_val);

    /* getpriority: PRIO_PROCESS = 进程级别 */
    int prio = getpriority(PRIO_PROCESS, 0);
    printf("getpriority(PRIO_PROCESS, 0) = %d\n", prio);

    /* setpriority: 提高/降低优先级（普通用户只能降低） */
    if (setpriority(PRIO_PROCESS, 0, 5) == 0)
        printf("Nice set to 5 (lower priority)\n");
    else
        perror("setpriority (need root to increase)");

    /* 调度策略: SCHED_OTHER (默认) / SCHED_FIFO / SCHED_RR */
    int policy = sched_getscheduler(0);
    printf("Scheduling policy: %d", policy);
    if (policy == SCHED_OTHER) printf(" (SCHED_OTHER - normal)\n");
    else if (policy == SCHED_FIFO) printf(" (SCHED_FIFO - realtime)\n");
    else if (policy == SCHED_RR) printf(" (SCHED_RR - round robin)\n");

    /* CPU 亲和性: 绑定到 CPU 0 */
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0)
        printf("Pinned to CPU 0\n");
    else
        perror("sched_setaffinity");

    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
