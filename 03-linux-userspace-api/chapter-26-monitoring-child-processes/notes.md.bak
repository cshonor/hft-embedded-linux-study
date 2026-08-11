# TLPI 第 26 章 — Monitoring Child Processes

> 对应目录：`chapter-26-monitoring-child-processes/`  
> 书名原文：**Monitoring Child Processes**  
> ⚠️ **僵尸靠 `wait*` 收割。** `SIGCHLD` 不排队 → handler 内必须 `while (waitpid(-1, …, WNOHANG) > 0)`。

**优先级**：🔴（多进程服务、防僵尸耗尽 PID）  
**前置**：[Ch25 进程终止](../chapter-25-process-termination/notes.md) · [Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch21 SIGCHLD](../chapter-21-signal-handlers/notes.md)  
**后置**：[Ch27 exec](../chapter-27-program-execution/notes.md) · [Ch37 守护进程](../chapter-37-daemons/notes.md)

---

## 章节目标

掌握 `wait`/`waitpid`/`waitid`；解析 `wstatus`；用 `SIGCHLD`+循环 `WNOHANG` 防漏收；分清阻塞/非阻塞与停止/继续选项。

---

## 26.1 僵尸

子终止 → 资源大多释放，内核表留 PID/退出状态 → **zombie** → 父 `wait*` 后条目删除。  
父先退 → init/systemd 收养并 wait。长期不收 → PID 耗尽风险。

---

## 26.2 `wait`

```c
pid_t wait(int *wstatus);   /* 阻塞至任意子终止；无子 → -1 */
```

不能指定 PID、无 `WNOHANG`、默认不管 stop/continue → 业务优先 `waitpid`。

---

## 26.3 `waitpid`（核心）

```c
pid_t waitpid(pid_t pid, int *wstatus, int options);
```

### `pid`

| 值 | 含义 |
|----|------|
| `> 0` | 指定子 PID |
| `0` | 同进程组任意子 |
| `< -1` | 进程组 `\|pid\|` 内任意子 |
| `-1` | 任意子（≈ `wait`） |

### `options`

| 标志 | 含义 |
|------|------|
| `WNOHANG` | 非阻塞；无就绪返回 `0` |
| `WUNTRACED` | 也报被停的子 |
| `WCONTINUED` | 也报 `SIGCONT` 恢复 |

返回：`>0` 子 PID · `0`（仅 WNOHANG）· `-1` 出错。

Demo：[`code/waitpid_status.c`](./code/waitpid_status.c)

---

## 26.4 `wstatus` 宏（必用宏，勿裸打数值）

| 宏 | 含义 |
|----|------|
| `WIFEXITED` → `WEXITSTATUS` | 正常退出 + 退出码 |
| `WIFSIGNALED` → `WTERMSIG` | 信号杀死；`WCOREDUMP`（Linux） |
| `WIFSTOPPED` → `WSTOPSIG` | 暂停（需 WUNTRACED） |
| `WIFCONTINUED` | 继续（需 WCONTINUED） |

仅当 `WIFEXITED` 为真时用 `WEXITSTATUS`。

---

## 26.5 `SIGCHLD` 循环收割（经典）

标准信号**不排队** → 一次 handler 可能对应多个子退出。

```c
while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
    /* handle */
}
```

| 点 | |
|----|--|
| 默认 | SIGCHLD 默认忽略；要收须 `sigaction` |
| `SA_NOCLDWAIT` | 内核自收、无僵尸；通常也不再靠 SIGCHLD 拿状态 |
| handler | 尽量轻；只收割或设标志 |

Demo：[`code/sigchld_reap_loop.c`](./code/sigchld_reap_loop.c)

---

## 26.6 `waitid`（了解）

```c
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
```

`P_PID` / `P_PGID` / `P_ALL`；信息更细。兼容性上 `waitpid` 仍最广。

---

## 26.7 规则要点

- 只能 wait **直系子**  
- 已回收不能再 wait  
- 已是僵尸 → waitpid 立即返回  
- 多线程：任一线程可 waitpid  

---

## 26.8 易错清单

1. SIGCHLD 只 wait 一次 → 漏僵尸  
2. 裸打 `wstatus`  
3. 默认 waitpid **只**看终止（除非 WUNTRACED/CONTINUED）  
4. `SA_NOCLDWAIT` 后别再指望拿退出状态/SIGCHLD  
5. 信号杀时勿用 `WEXITSTATUS`  

---

## 速查：`pid` / 状态

| waitpid pid | 对象 |
|-------------|------|
| `>0` / `0` / `<-1` / `-1` | 指定 / 同组 / 指定组 / 全部 |

| 想知道 | 先查 |
|--------|------|
| 退出码 | `WIFEXITED` + `WEXITSTATUS` |
| 哪个信号杀的 | `WIFSIGNALED` + `WTERMSIG` |

---

## 练习

1. 阻塞 waitpid + 解析退出码  
2. 子自杀 `SIGTERM`/`abort`，看 `WIFSIGNALED`  
3. SIGCHLD 循环 `WNOHANG`  
4. （选）`SA_NOCLDWAIT` 对比  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 僵尸 = 等父 wait 的表项 |
| 2 | 用 `waitpid`，别只用 `wait` |
| 3 | 状态必须用 `WIF*` 宏 |
| 4 | SIGCHLD：`while` + `WNOHANG` |
| 5 | `WNOHANG` 返回 0 = 暂无就绪 |
| 6 | 只能收直系子 |

---

## 参考

- Kerrisk · TLPI Ch26  
- `man 2 wait` · `man 2 waitpid` · `man 2 waitid` · `man 7 signal`（SIGCHLD）
