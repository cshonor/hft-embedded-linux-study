# TLPI 第 12 章 — System and Process Information

**优先级**：🔴（监控 / 调试 / 嵌入式与 HFT 观测常读 `/proc`）  
**前置**：[Ch11 System Limits](../chapter-11-system-limits/notes.md)（`sysconf` 的补充来源）  
**后置**：[Ch13 File I/O Buffering](../chapter-13-file-io-buffering/notes.md) · 读 `/proc` 依赖的 **open/read** 见 [Ch4](../chapter-04-file-io-universal/notes.md)  

---

## 小节目录

- [12.1 `uname()` — POSIX](./notes/12.1-uname.md)
- [12.2 `/proc` — Linux 核心特色（非 POSIX）](./notes/12.2-proc.md)
- [12.3 读 `/proc` 编程要点](./notes/12.3-proc.md)
- [12.4 `sysctl()` 系统调用](./notes/12.4-sysctl.md)
- [12.5 主机名 / 域名](./notes/12.5-section-12-5.md)
- [12.6 `sysinfo()` — Linux](./notes/12.6-sysinfo.md)
- [12.7 `get_nprocs()` — GNU](./notes/12.7-getnprocs-gnu.md)
- [12.8 可移植 vs Linux](./notes/12.8-section-12-8.md)
- [12.9 易错考点](./notes/12.9-section-12-9.md)

---

## 章节目标


获取系统软硬件与进程运行信息；会用 `uname`；会解析 `/proc` 全局文件与 `/proc/[pid]`；分清可移植 API 与 Linux 专属接口；为进程管理、监控、调试打底。

---


---

## `/proc` 速查（背诵）


| 要什么 | 看哪里 |
|--------|--------|
| 内核版本 | `uname` 或 `/proc/version` |
| 内存 | `/proc/meminfo`（或谨慎用 `sysinfo`） |
| CPU | `/proc/cpuinfo`、`get_nprocs` |
| 负载 | `/proc/loadavg` |
| 本进程状态 | `/proc/self/status` |
| 本进程映射 | `/proc/self/maps` |
| 本进程 fd | `/proc/self/fd/` |
| 调参 | `/proc/sys/...` + `sysctl` |

---


---

## 与前后章


| 章 | 关联 |
|----|------|
| Ch11 | `sysconf`；本章 `/proc` 给更细粒度 |
| Ch4 / Ch13 | 读 `/proc` = 普通文件 I/O + 缓冲细节 |
| Ch24 / Ch35 | 进程列表、调度观测 |
| Ch49 | `/proc/[pid]/maps` 对照地址空间 |

---


---

## 练习


1. `uname` 打印系统信息  
2. 解析 `/proc/self/status`（UID、VmRSS…）  
3. 遍历 `/proc` 数字目录 → 迷你 `ps`  
4. `loadavg` + `meminfo` 简易监控  
5. 枚举 `/proc/self/fd`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 可移植：`uname`；详情：`/proc` |
| 2 | `/proc/self` = 当前进程 |
| 3 | `cmdline`/`environ` 用 `\0` 分隔 |
| 4 | `stat` 的 `(comm)` 难解析；优先 `status` |
| 5 | 勿 mmap `/proc`；进程可能已消失 |
| 6 | 容器内 `/proc` 是隔离视图 |

---


---

## 参考


- Kerrisk · TLPI Ch12  
- `man 2 uname` · `man 5 proc` · `man 2 sysinfo` · `man 7 sysctl`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>

/* Ch12 系统与进程信息 — uname/sysinfo + /proc 文件系统。
 * 编译: gcc -o ch12_demo ch12_demo.c */

int main(void) {
    /* uname: 内核/系统信息 */
    struct utsname uts;
    if (uname(&uts) == 0) {
        printf("sysname:  %s\n", uts.sysname);
        printf("nodename: %s\n", uts.nodename);
        printf("release:  %s\n", uts.release);
        printf("version:  %s\n", uts.version);
        printf("machine:  %s\n", uts.machine);
    }

    /* sysinfo: 内存/uptime/负载 */
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        printf("\nuptime:      %ld sec\n", info.uptime);
        printf("total RAM:   %lu MB\n", info.totalram / 1024 / 1024);
        printf("free RAM:    %lu MB\n", info.freeram / 1024 / 1024);
        printf("load avg:    %.2f %.2f %.2f\n",
               info.loads[0] / 65536.0,
               info.loads[1] / 65536.0,
               info.loads[2] / 65536.0);
    }

    printf("\n/proc/self/status shows per-process info (try: cat /proc/self/status)\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
