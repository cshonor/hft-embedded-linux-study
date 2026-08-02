# TLPI 第 28 章 — Process Creation and Program Execution in More Detail

> 对应目录：`chapter-28-process-creation-exec-detail/`  
> （勿用 `chapter-28-process-fork-exec-deep-dive` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 不一致）  
> 书名原文：**Process Creation and Program Execution in More Detail**  
> ⚠️ **本章是 fork/exec 深潜，不是凭证。** 凭证 = [Ch9](../chapter-09-process-credentials/notes.md)。

**编号纠偏（相对常见错位大纲）：**

| 大纲误标 | Kerrisk / 本仓库 |
|----------|------------------|
| Ch28 = 凭证 | ❌ → **Ch9** Process Credentials |
| Ch29 = 凭证 / 调度 | ❌ → **Ch29** Threads 导论；调度 = **Ch35** |
| Ch28 = fork/exec 细节 | ✅ 本章 |

**优先级**：🔴（多线程 fork、信号/fd 生命周期、clone 关系）  
**前置**：[Ch24](../chapter-24-process-creation/notes.md) · [Ch25](../chapter-25-process-termination/notes.md) · [Ch26](../chapter-26-monitoring-child-processes/notes.md) · [Ch27](../chapter-27-program-execution/notes.md)  
**后置**：[Ch29 线程导论](../chapter-29-threads-intro/notes.md) · 凭证细节见 [Ch9](../chapter-09-process-credentials/notes.md)

---

## 章节目标

深挖 COW 与继承清单；多线程 fork / `pthread_atfork`；`vfork`/`clone`；exec 资源细则与 fork/exec 信号对比表；`O_CLOEXEC`；规范 fork+exec。

---

## 28.1 fork 再深入

### COW

fork 复制页表、页标只读；写时缺页再拷贝物理页。现代 Linux fork 开销低；逻辑上地址空间仍独立。

### 继承 vs 不继承（精要）

| ✅ 继承 | ❌ 不继承 |
|---------|-----------|
| 地址空间（COW）、fd 表（共享 `struct file`/偏移） | 新 PID/PPID |
| 信号 **handler**、信号 **掩码** | **pending 清空** |
| 凭证、cwd、umask、rlimit、PGID/SID… | 其它线程（只留调用线程） |
| | 文件锁、部分定时器/AIO/inotify 等 |

> 掩码继承；pending 清空。

Demo：[`code/fork_signal_state.c`](./code/fork_signal_state.c)

---

## 28.2 多线程 + fork

POSIX：子进程**只保留发起 fork 的线程**；其它线程不跑清理 → 锁/堆状态危险。

| 方案 | |
|------|--|
| **最优** | fork 后立刻 exec |
| 缓解 | `pthread_atfork(prepare, parent, child)` 统一加解锁 |
| 避免 | fork 后子进程继续跑复杂多线程逻辑 |

---

## 28.3 `vfork`

共享地址空间；父阻塞至子 `_exit`/`exec`；子勿乱改内存。新代码用 fork+COW，**禁用 vfork**。

---

## 28.4 `clone`（Linux）

`fork` / `pthread_create` 均基于 `clone` 标志组合：

| 标志例 | 含义 |
|--------|------|
| `CLONE_VM` | 共享地址空间（线程） |
| `CLONE_FILES` | 共享 fd 表 |
| `CLONE_SIGHAND` | 共享信号处置 |

`fork` ≈ 不共享上述、并带 `SIGCHLD` 的 clone 特例。

---

## 28.5 exec 资源细则（承 Ch27）

| ✅ 保留 | ❌ 销毁/重置 |
|---------|--------------|
| PID/PGID/SID、cwd、umask | 整块用户地址空间 |
| **信号掩码** | **handler → SIG_DFL** |
| 未 CLOEXEC 的 fd | pending 清空、atexit 丢弃 |
| rlimit… | 其它线程、多数锁/定时器/AIO… |

### 超级对比表

| 操作 | 信号掩码 | handler | pending |
|------|----------|---------|---------|
| **fork** | 继承 | 继承 | **清空** |
| **exec** | **继承** | **→ SIG_DFL** | **清空** |

---

## 28.6 `FD_CLOEXEC` / `O_CLOEXEC`

| | fork | exec |
|--|------|------|
| CLOEXEC fd | 不关（标记可复制） | **关掉** |

打开即 `O_CLOEXEC`，避免 fork 与 exec 之间的竞态。见 [Ch27 cloexec demo](../chapter-27-program-execution/code/cloexec_demo.c)。

---

## 28.7 fork + exec 规范

```c
if (fork() == 0) {
    /* redirect / close fds */
    execvp(...);
    _exit(127);   /* 禁止 exit() */
}
```

Demo：[`code/fork_exec_redirect.c`](./code/fork_exec_redirect.c)

---

## 28.8 shebang

`#!` → 内核转解释器；Linux 上脚本 **SUID 无效**（见 Ch9）。

---

## 28.9 易错清单

1. fork：掩码继承、pending 清；exec：handler→DFL、掩码保留  
2. 多线程 fork → 立刻 exec  
3. 勿用 vfork  
4. CLOEXEC 只对 exec  
5. exec 成功不返回；失败 `_exit`  
6. clone 是 fork/线程共同底座  

---

## 实验清单

1. COW：改全局变量互不影响（见 Ch24 `fork_basic`）  
2. （选）多线程 fork 风险  
3. CLOEXEC（Ch27）  
4. fork+exec 重定向模板  
5. fork/exec 信号状态对比（本目录 demo）  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | fork 掩码✓ pending✗；exec 掩码✓ handler→DFL |
| 2 | 多线程 fork 只留一线程 → 立刻 exec |
| 3 | vfork 共享地址空间，新代码禁用 |
| 4 | fork/线程 ⊂ clone(flags) |
| 5 | CLOEXEC：fork 不关、exec 关 |
| 6 | 子失败 `_exit`，防 atexit 双跑 |

---

## 参考

- Kerrisk · TLPI Ch28  
- `man 2 fork` · `man 2 vfork` · `man 2 clone` · `man 2 execve` · `man 3 pthread_atfork`
