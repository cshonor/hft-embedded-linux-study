# TLPI 第 24 章 — Process Creation

> 对应目录：`chapter-24-process-creation/`  
> 书名原文：**Process Creation**  
> ⚠️ **一次 `fork`，两处返回。** COW 只是优化；子进程 **pending 清空**、掩码继承。多线程 `fork` 只留调用线程 → 易死锁。

**优先级**：🔴（shell、服务、多进程模型地基）  
**前置**：[Ch23 Timers](../chapter-23-timers-sleeping/notes.md) · [Ch20–22 信号](../chapter-20-signals-fundamentals/notes.md)  
**后置**：[Ch25 进程终止](../chapter-25-process-termination/notes.md) · [Ch26 wait](../chapter-26-monitoring-child-processes/notes.md) · [Ch27 exec](../chapter-27-program-execution/notes.md)

---

## 章节目标

掌握 `fork`/COW；理清继承与不继承；fd 共享与 stdio 缓冲陷阱；多线程 `fork` 风险与 `fork+exec` 范式；了解为何少用 `vfork`。

---

## 24.1 `fork()`

```c
#include <unistd.h>
pid_t fork(void);
```

| 返回 | 含义 |
|------|------|
| `> 0` | 父进程：子 PID |
| `0` | 子进程 |
| `-1` | 失败 |

Demo：[`code/fork_basic.c`](./code/fork_basic.c)

---

## 24.2 Copy-On-Write（COW）

fork 瞬间共享页（标只读）；一方写入 → 缺页 → 内核复制该页。  
逻辑上地址空间独立；COW 只是加速。

---

## 24.3 继承清单（速查）

### ✅ 大致继承 / 共享语义

虚拟地址（COW）· fd 表（打开文件引用 +1，共享偏移）· cwd / umask / PGID / SID · 信号处置 · 凭证 · rlimit · 信号**掩码** 等

### ❌ 不继承 / 特殊

| 项 | 行为 |
|----|------|
| PID / PPID | 新值 |
| **pending 信号** | **清空**（高频考点） |
| 其它线程 | **全部消失**（只留调用 `fork` 的线程） |
| 文件锁 | 通常不继承 |
| 部分定时器 / inotify 等 | 实现相关，勿假设完整继承 |

---

## 24.4 文件描述符

父子指向同一打开文件描述 → **共享文件偏移**与状态标志。  
策略：用完 `close`；`FD_CLOEXEC` 只在 **exec** 时关，**fork 不会**因它关 fd。

Demo：[`code/fork_fd_offset.c`](./code/fork_fd_offset.c)

---

## 24.5 stdio 缓冲陷阱

`printf` 无 `\n`（全缓冲时）→ 用户缓冲未刷 → fork **复制缓冲** → 父子各打一份。  
**fork 前 `fflush(NULL)` / `fflush(stdout)`。**

Demo：[`code/fork_stdio_buf.c`](./code/fork_stdio_buf.c)

---

## 24.6 多线程 + `fork`（重难点）

子进程只保留 **fork 那条线程**；其它线程不跑清理 → 锁可能永久锁死、堆状态撕裂。

| 做法 | |
|------|--|
| ✅ | 多线程里尽量只 `fork` 后立刻 `exec` |
| ⚠️ | `pthread_atfork` 只能缓解 |
| ❌ | fork 后子进程继续跑复杂多线程逻辑 |

---

## 24.7 典型范式

**fork + exec**（shell / 起外部程序）：子进程关多余 fd、重定向，再 `exec*`；失败 `_exit(127)`。  
**fork 后分叉业务**（多进程服务）：父子各跑逻辑；父须 `wait*` 防僵尸（Ch26）。

---

## 24.8 `vfork`（了解即可）

共享地址空间、父阻塞到子 `_exit`/`exec`；子乱改内存即毁父。现代 COW 已够用 → **新代码勿用 `vfork`**。

---

## 24.9 易错清单

1. pending 清空，掩码继承  
2. COW ≠ 共享可写内存  
3. fork 前 fflush  
4. 多线程 fork → 死锁风险  
5. fd 共享偏移  
6. 子退出需 `wait`（Ch26）  

---

## 练习

1. 打印父子 PID / 返回值  
2. 改全局变量，验证互不影响  
3. 复现/修复 stdio 双份输出  
4. （选）多线程 fork 风险  
5. 父子同 fd `write` 交错  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 一调两返：父=子 PID，子=0 |
| 2 | COW：写时才真正分页 |
| 3 | 子 pending 清空；掩码继承 |
| 4 | 多线程 fork 只留一线程 |
| 5 | fork 前 fflush；fd 共享偏移 |
| 6 | 首选 fork+exec；少用 vfork |

---

## 参考

- Kerrisk · TLPI Ch24  
- `man 2 fork` · `man 2 vfork` · `man 3 pthread_atfork`
