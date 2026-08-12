# TLPI 第 36 章 — Process Resources

**优先级**：🔴（服务调 `NOFILE`、剖析 CPU/RSS、daemon 启动设限）  
**前置**：[Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md)  
**后置**：[Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 小节目录

- [36.1 `getrusage`](./notes/36.1-getrusage.md)
- [36.2 `getrlimit` / `setrlimit`](./notes/36.2-getrlimit-setrlimit.md)

---

## 章节目标


`getrusage`；软/硬 `rlimit`；超限信号/错误；fork/exec 继承；与 shell `ulimit` / systemd 的关系。

---


---

## 易错清单


1. CHILDREN 必须 wait 才进账  
2. `ru_maxrss` 是峰值  
3. 硬限降了回不去（非特权）  
4. `NOFILE` 与「最大 fd 编号」关系：大约 `cur-1`  
5. 启动时主动抬软限是常见服务套路  

---


---

## 实验清单


1. `getrusage` 看 CPU/RSS/切换  
2. CHILDREN + wait  
3. 打印默认 rlimit  
4. 抬高 `RLIMIT_NOFILE`（不超硬限）  
5. （选）硬限不可升  

---


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


---

## 参考


- Kerrisk · TLPI Ch36  
- `man 2 getrusage` · `man 2 getrlimit` · `man 2 setrlimit` · `man 2 prlimit`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

/* Ch36 进程资源 — getrusage/setrlimit/getrlimit。
 * 演示资源使用统计 + 资源限制设置。
 * 编译: gcc -o ch36_demo ch36_demo.c */

int main(void) {
    /* getrusage: 获取资源使用情况 */
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("Resource usage (self):\n");
        printf("  user CPU time:   %ld.%06ld sec\n",
               (long)usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec);
        printf("  system CPU time: %ld.%06ld sec\n",
               (long)usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec);
        printf("  max RSS:         %ld KB\n", (long)usage.ru_maxrss);
        printf("  minor faults:    %ld\n", (long)usage.ru_minflt);
        printf("  major faults:    %ld\n", (long)usage.ru_majflt);
        printf("  voluntary CS:    %ld\n", (long)usage.ru_nvcsw);
        printf("  involuntary CS:  %ld\n", (long)usage.ru_nivcsw);
    }

    /* getrlimit/setrlimit: 资源限制 */
    struct rlimit lim;
    getrlimit(RLIMIT_NOFILE, &lim);
    printf("\nRLIMIT_NOFILE: soft=%lu, hard=%lu\n",
           (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);

    getrlimit(RLIMIT_STACK, &lim);
    printf("RLIMIT_STACK:   soft=%lu, hard=%lu\n",
           (unsigned long)lim.rlim_cur, (unsigned long)lim.rlim_max);

    /* 降低文件描述符上限 */
    lim.rlim_cur = 256;
    if (setrlimit(RLIMIT_NOFILE, &lim) == 0) {
        getrlimit(RLIMIT_NOFILE, &lim);
        printf("After setrlimit: soft=%lu\n", (unsigned long)lim.rlim_cur);
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
