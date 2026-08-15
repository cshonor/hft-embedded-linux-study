# TLPI 第 06 章 — Processes

**优先级**：🔴（后续 fork/信号/多进程的地基）  
**前置**：[Ch3](../chapter-03-system-programming-concepts/README.md) · [Ch4](../chapter-04-file-io-universal/README.md) · [Ch5](../chapter-05-file-io-further/README.md)  
**后置**：[Ch7 内存分配](../chapter-07-memory-allocation/README.md) · [Ch8 用户与组](../chapter-08-users-and-groups/README.md) · [Ch24 fork](../chapter-24-process-creation/README.md)  

---

## 小节目录

- [6.1 进程基本概念](./notes/6.1-basic-concepts-process.md)
- [6.2 PID 与 PPID](./notes/6.2-pid-ppid.md)
- [6.3 进程虚拟地址空间（本章核心）](./notes/6.3-address-space-process.md)
- [6.4 命令行参数 `argv`](notes/6.4-virtual-memory.md)
- [6.5 环境变量](notes/6.5-stack-frames.md)
- [6.6 非局部跳转 `setjmp` / `longjmp`](notes/6.8-setjmp-longjmp.md)

---

## 章节目标


建立进程基础模型：虚拟地址空间布局、命令行参数、环境变量、PID/PPID；掌握非局部跳转 `setjmp`/`longjmp`，为后续 fork、exec、信号、多进程铺路。

---


---

## 易错清单


1. BSS 在磁盘可执行文件几乎不占空间；加载后分配并清零。  
2. 代码段 / 字符串字面量只读；改写 → 常 `SIGSEGV`。  
3. `getenv` 指针勿 `free`。  
4. `putenv` 传入局部栈缓冲 → 野指针风险。  
5. 信号处理里 `longjmp` 有额外限制（Ch21）。  
6. 用 `environ`，勿依赖非标准 `envp`。  
7. 本章 **没有** `fork`/`exec`；别和 Ch24–27 混章。

---


---

## 章节链路


```
Ch5  fd / 打开描述（stdin/out/err 已在进程里）
  → Ch6  进程模型、地址空间、环境、setjmp
  → Ch7  堆 / brk / malloc
  → Ch8  UID/GID
  → Ch24 fork：复制地址空间、环境、fd 表（本章直接落地）
```

---


---

## 双线提示


| 路线 | |
|------|--|
| 嵌入式 | 搞清栈/堆/BSS；环境变量做配置；少依赖 `putenv` |
| HFT | 地址空间与后续 `mmap`/大页衔接；`setjmp` 少用在热路径 |

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 程序=文件；进程=运行实例 + `task_struct` |
| 2 | Text / Data / BSS / Heap↑ / Stack↓ |
| 3 | `getpid`/`getppid` 永不失败；孤儿 → 常 PPID=1 |
| 4 | `environ` + `setenv`；改环境不影响父 shell |
| 5 | `setjmp` 返回 0；`longjmp` 返回 val；中间变量用 `volatile` |
| 6 | fork/exec 在 Ch24–27，不在本章 |

---


---

## 参考


- Kerrisk, *The Linux Programming Interface*, **Chapter 6 — Processes**  
- [OUTLINE](../OUTLINE.md) · [Ch5](../chapter-05-file-io-further/README.md) · [Ch7](../chapter-07-memory-allocation/README.md) · [Ch24](../chapter-24-process-creation/README.md)


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Ch6 进程 — 演示 fork/wait/getpid 基本进程操作。
 * fork() 创建子进程；wait() 等待子进程结束。
 * 编译: gcc -o ch6_demo ch6_demo.c */

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* 子进程 */
        printf("Child: pid=%d, parent pid=%d\n",
               (int)getpid(), (int)getppid());
        _exit(42);
    } else {
        /* 父进程 */
        printf("Parent: pid=%d, child pid=%d\n",
               (int)getpid(), (int)pid);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            printf("Child exited with %d\n", WEXITSTATUS(status));
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
