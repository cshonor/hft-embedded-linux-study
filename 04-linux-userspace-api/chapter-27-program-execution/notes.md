# TLPI 第 27 章 — Program Execution

> 对应目录：`chapter-27-program-execution/`  
> 书名原文：**Program Execution**  
> ⚠️ **`fork` 造新进程；`exec` 在同一 PID 上换程序镜像。** 成功不返回；失败才 `-1`，子进程用 `_exit`。

**优先级**：🔴（shell、服务拉起外部程序、fork+exec 标准模型）  
**前置**：[Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch25 终止](../chapter-25-process-termination/notes.md) · [Ch26 wait](../chapter-26-monitoring-child-processes/notes.md)  
**后置**：[Ch28 fork/exec 细节](../chapter-28-process-creation-exec-detail/notes.md) · [Ch9 凭证 / SUID](../chapter-09-process-credentials/notes.md)

> 本书编号：凭证在 **Ch9**；Ch28 是 fork/exec 更细规则。用户大纲里的「Ch28 凭证」按仓库 CHAPTER-MAP 对应到 Ch9。

---

## 章节目标

掌握 exec 六兄弟与 `execve`；理清保留/销毁资源；`FD_CLOEXEC` / `O_CLOEXEC`；PATH 与 shebang；熟练 **fork + exec + waitpid**。

---

## 27.1 六兄弟

真正的系统调用是 **`execve`**；其余为库封装。

| 后缀 | 含义 |
|------|------|
| `l` | 参数列表，末尾 `(char *)NULL` |
| `v` | `argv[]` 向量 |
| `p` | 用 `PATH` 搜文件名 |
| `e` | 自带 `envp[]`；否则继承 `environ` |

```c
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execl / execlp / execle(...);
int execv / execvp(...);
```

**成功永不返回；失败返回 -1。**

```c
execvp("ls", argv);
_exit(127);
```

Demo：[`code/fork_exec.c`](./code/fork_exec.c)

---

## 27.2 保留 vs 销毁

### ✅ 大致保留

PID/PPID/PGID/SID · cwd · umask · **未设 CLOEXEC 的 fd** · 凭证（SUID 见 Ch9）· **信号掩码** · rlimit 等

### ❌ 销毁 / 重置

整块地址空间 · 其它线程 · **信号处理器 → `SIG_DFL`** · pending 清空 · atexit 清空 · 多数定时器/文件锁/AIO 等

| | fork | exec |
|--|------|------|
| 信号 handler | 继承 | **重置默认** |
| 信号掩码 | 继承 | **保留** |
| pending | 清空 | 清空 |

---

## 27.3 `FD_CLOEXEC` / `O_CLOEXEC`

| | fork | exec |
|--|------|------|
| CLOEXEC fd | **不关** | **内核关掉** |

打开时带 `O_CLOEXEC`（首选，减竞态）；或 `fcntl(F_SETFD, FD_CLOEXEC)`。

Demo：[`code/cloexec_demo.c`](./code/cloexec_demo.c)

---

## 27.4 PATH（`*p`）

- 名中无 `/` → 按 `PATH` 搜  
- 含 `/` → 当路径，不搜 PATH  
- 特权程序慎用 `*p`（PATH 含 `.` 等劫持风险）

---

## 27.5 shebang `#!`

`execve` 见 `#!` → 跑解释器 + 脚本路径为参数。脚本建议 `+x`。

---

## 27.6 工业范式：fork + exec

```c
pid = fork();
if (pid == 0) {
    /* close / redirect fds */
    execvp(prog, argv);
    _exit(127);          /* 禁止 exit() */
}
waitpid(pid, &st, 0);    /* 父 */
```

立刻 exec → COW 几乎不触发；多线程场景也相对安全。

---

## 27.7 环境变量

无 `e`：用 `environ`。有 `e`：自定义 `envp`，末项 `NULL`。  
特权启动宜清理危险变量（如 `LD_*`）。

---

## 27.8 易错清单

1. exec 成功后无后续业务代码  
2. handler 重置；掩码保留  
3. CLOEXEC 只对 exec  
4. root + `execvp` PATH 风险  
5. 失败用 `_exit`  
6. exec **不改 PID**；新进程靠 fork  
7. `argv[0]` 约定为名，内核不强制  

---

## 练习 / 实验清单

1. `execvp` / `execl`  
2. `FD_CLOEXEC` 跨 exec  
3. （选）shebang  
4. fork+exec+重定向模板  
5. （选）exec 前后 handler 对比  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | exec = 同 PID 换镜像；成功不返回 |
| 2 | 真 syscall：`execve`；l/v/p/e 记口诀 |
| 3 | handler→DFL；掩码保留；pending 清 |
| 4 | CLOEXEC：fork 不关，exec 关 |
| 5 | 子失败 `_exit`；父 `waitpid` |
| 6 | 新 PID 只来自 fork |

---

## 参考

- Kerrisk · TLPI Ch27  
- `man 3 exec` · `man 2 execve` · `man 2 fcntl`（`FD_CLOEXEC`）
