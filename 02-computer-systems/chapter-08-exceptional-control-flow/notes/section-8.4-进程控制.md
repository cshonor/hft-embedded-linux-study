## 8.4 进程控制（8.4.1–8.4.6）

### 8.4.1 获取进程 ID

```c
pid_t getpid(void);   // 本进程
pid_t getppid(void);  // 父进程
```

### 8.4.2 创建和终止进程

```c
pid_t fork(void);     // 子进程返回 0，父返回子 pid
void exit(int status);
```

- **`fork`** — **写时复制 (COW)** 复制地址空间；子继承 fd 表副本
- **`exit`** — 内核记录 **退出状态**，进程变 **僵尸** 直到被回收

### 8.4.3 回收子进程

```c
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

- **僵尸 (zombie)** — 已 exit 但未 `wait`；占 pid 不释放
- **孤儿** — 父先死，由 **init/systemd** 收养并回收
- **`SIGCHLD`** — 子状态变化时通知父（常配合非阻塞 `waitpid`）

### 8.4.4 让进程休眠

```c
unsigned sleep(unsigned seconds);
int pause(void);           // 直到信号
int sigsuspend(...);       // 原子地阻塞并等待
```

### 8.4.5–8.4.6 加载运行：`execve` 与 shell 模式

```c
int execve(const char *path, char *const argv[], char *const envp[]);
```

- **替换** 当前进程映像；**不创建新进程** — 成功则不返回
- 典型：**`fork` + `execve`** — shell 启动命令

```c
if ((pid = fork()) == 0) {
    execve("/bin/ls", argv, envp);
    _exit(1);
}
waitpid(pid, NULL, 0);
```

**HFT：**

- **不在 tick 路径 `fork/exec`** — 毫秒级、cache 冷、不可控
- 用于 **守护进程化、子进程跑脚本、隔离工具**；主引擎 **长期运行单进程**
- 理解 `execve` → 与 [Ch 7 加载](../../chapter-07-linking/notes/section-7.9-加载可执行目标文件.md) 衔接

### 自测题

<details>
<summary>1. fork() 之后父子进程的关系？哪些是共享的哪些是复制的？</summary>

fork 后子进程获得父进程的**副本**（地址空间、文件描述符表、信号处理函数）。但用的是 **COW（Copy-On-Write）**——只复制页表不复制物理页，首次写才分配新页。文件描述符表是复制的（共享 offset），但引用计数 +1。父子进程各自返回——子进程返回 0，父进程返回子进程 PID。

</details>

<details>
<summary>2. execve() 做了什么？它和 fork() 的关系？</summary>

`execve` 在**当前进程**的上下文中加载并运行一个新程序——替换代码段、数据段、堆、栈，但保留 PID、打开的文件描述符（除非设了 FD_CLOEXEC）。典型模式：`fork()` 创建子进程 → `execve()` 替换为新程序。fork 负责复制，execve 负责替换，两者组合实现「启动新程序」。

</details>

<details>
<summary>3. waitpid() 的作用是什么？为什么需要它？</summary>

`waitpid` 让父进程等待子进程结束并回收其资源（避免僵尸进程）。子进程结束后如果父进程没调用 wait，子进程变成**僵尸进程（Z 状态）**——PCB 仍占用，直到父进程 wait 或父进程退出被 init 接管。

</details>


---

← [本章导读](../README.md)
