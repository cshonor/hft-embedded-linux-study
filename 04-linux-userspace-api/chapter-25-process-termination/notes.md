# TLPI 第 25 章 — Process Termination

> 对应目录：`chapter-25-process-termination/`  
> 书名原文：**Process Termination**  
> ⚠️ **`exit` = atexit + 刷 stdio + `_Exit`；`_exit`/`_Exit` 直进内核。** fork 子进程（不 exec）一律 `_exit`，防 atexit/缓冲双份。

**优先级**：🔴（fork 后退出选型、退出码、僵尸衔接）  
**前置**：[Ch24 fork](../chapter-24-process-creation/notes.md)  
**后置**：[Ch26 wait / 僵尸](../chapter-26-monitoring-child-processes/notes.md) · [Ch27 exec](../chapter-27-program-execution/notes.md)

---

## 章节目标

区分正常/异常终止；对比 `exit` / `_exit` / `_Exit`；掌握退出状态与 atexit；理解内核回收与僵尸；衔接 wait。

---

## 25.1 终止分类

| 类型 | 方式 |
|------|------|
| **正常** | `main` return · `exit` · `_exit` / `_Exit` |
| **异常** | 致命信号 · `abort()` → `SIGABRT` |

最终都进内核销毁路径。

---

## 25.2 `exit` vs `_exit` / `_Exit`（核心）

```c
void exit(int status);    /* stdlib：库清理后进内核 */
void _Exit(int status);   /* C99：直进内核 */
void _exit(int status);   /* POSIX：与 _Exit 等价（Linux） */
```

| 步骤 | `exit` | `_exit` / `_Exit` |
|------|--------|-------------------|
| atexit / on_exit | ✅ 逆序跑 | ❌ |
| 刷 stdio | ✅ | ❌（用户缓冲可能丢） |
| 进内核销毁 | ✅ | ✅ |

### fork 规范

```c
if (fork() == 0)
    _exit(EXIT_FAILURE);   /* 不 exec 时禁止 exit() */
```

fork 复制了 atexit 列表与 stdio 缓冲 → 子调 `exit` 易重复回调/重复刷缓冲。

Demo：[`code/exit_vs_exit.c`](./code/exit_vs_exit.c) · [`code/fork_atexit.c`](./code/fork_atexit.c)

---

## 25.3 `main` return

`return N;` ≈ `exit(N)`。无 return 时 C99+ 默认 `return 0`。

---

## 25.4 退出状态

仅**低 8 位**有效（0–255）。  
`0` / `EXIT_SUCCESS` · 非 0 / `EXIT_FAILURE`。

父用 `wait`/`waitpid` 取状态：

| 宏 | 含义 |
|----|------|
| `WIFEXITED` | 正常终止 |
| `WEXITSTATUS` | 退出码（仅当 WIFEXITED） |
| （信号杀） | `WIFSIGNALED` 等；勿读 WEXITSTATUS |

---

## 25.5 `atexit` / `on_exit`

```c
int atexit(void (*func)(void));
int on_exit(void (*func)(int, void *), void *arg);  /* Linux 扩展 */
```

- 注册：**LIFO** 逆序执行  
- 仅 `exit` / `main` return 触发  
- `_exit` / 信号杀：**不跑**  
- fork 复制列表；exec 成功清空  
- 回调里勿再乱调 `exit`（易乱序/递归感）

Demo：[`code/atexit_order.c`](./code/atexit_order.c)

---

## 25.6 内核销毁时做什么

关 fd、放内存与多数资源 → 进程成**僵尸**（保留 PID/退出状态）→ 向父发 `SIGCHLD` → 父 `wait*` 才摘掉僵尸。

| | |
|--|--|
| 刷 **stdio 用户缓冲** | 仅 `exit` 路径 |
| 内核页缓存 / 关 fd | `_exit` 也会走内核清理（≠ 用户态 fflush） |

---

## 25.7 僵尸（衔 Ch26）

子先退、父未 wait → zombie。父退则由 init/systemd 收养并回收。

---

## 25.8 `abort`

发 `SIGABRT`；默认可 core；标准要求最终仍终止（即使用户 handler）。

---

## 25.9 易错清单

1. fork 子用 `exit` → atexit 双跑  
2. `_exit` 不刷 stdio → 输出可能丢  
3. 退出码截断到 8 位  
4. 信号杀跳过全部用户清理；`SIGKILL` 尤甚  
5. `on_exit` 不可移植；优先 `atexit`  

---

## 速查

| API | 回调 | 刷 stdio | 典型用途 |
|-----|------|----------|----------|
| `exit` | ✅ | ✅ | 正常结束进程 |
| `_exit`/`_Exit` | ❌ | ❌ | fork 子、信号敏感路径 |
| `abort` | 经信号 | — | 异常自毁 |

---

## 练习

1. atexit 逆序  
2. `exit` vs `_exit` 缓冲差异  
3. fork + `exit` 复现双 atexit；改 `_exit`  
4. `waitpid` + `WEXITSTATUS`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | `exit` = atexit + fflush + `_Exit` |
| 2 | fork 子（不 exec）用 `_exit` |
| 3 | 退出码仅低 8 位 |
| 4 | atexit LIFO；信号/`_exit` 不跑 |
| 5 | 内核收尸后僵尸等 wait |
| 6 | 用户缓冲 ≠ 内核刷盘 |

---

## 参考

- Kerrisk · TLPI Ch25  
- `man 3 exit` · `man 2 _exit` · `man 3 atexit` · `man 3 abort`
