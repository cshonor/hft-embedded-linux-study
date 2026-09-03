# 2.2 attach 运行中进程（gdb -p / ptrace 权限 / fork 跟随）

> 🔴 精读 · 常驻进程「不重启抓现场」

## 本节要点

很多 bug 只出现在**已经跑起来、且不能随便重启**的进程里——交易引擎、行情网关、守护进程。这类进程「卡住」「CPU 打满」「状态异常」时，`gdb -p <PID>` attach 上去直接看现场，比加日志重编译、或重启复现高效得多。本节讲 attach 的用法、ptrace 权限门槛、信号处理，以及 fork/exec 出来的子进程怎么跟。

## attach 的两种方式

```bash
# 方式 1：启动时就 attach
gdb -p 12345

# 方式 2：gdb 里 attach
gdb
(gdb) attach 12345
Attaching to process 12345
... 读取符号、加载线程 ...
(gdb) bt            # 立刻看它现在卡在哪
```

attach 后目标进程会**被 SIGSTOP 暂停**，进入 gdb 控制。看完现场：

```gdb
(gdb) detach         # 解除 attach，进程继续跑（脱离 gdb 控制）
(gdb) quit           # 若未 detach 直接 quit，gdb 会询问是否 kill 进程
```

> ⚠️ `quit` 前没 `detach`，gdb 会提示 `A debugging session is active` 并问是否终止进程——对生产进程这是事故。**养成先 `detach` 再 `quit` 的习惯**。

## ptrace 权限门槛（attach 失败的常见原因）

attach 依赖 `ptrace(2)`，而现代 Linux 用 Yama 限制谁能 ptrace 谁：

```bash
cat /proc/sys/kernel/yama/ptrace_scope
# 0 = 经典权限（同 uid 即可）
# 1 = 仅父子进程（默认，Ubuntu 等）
# 2 = 仅 root（CAP_SYS_PTRACE）
# 3 = 完全禁用 ptrace
```

| 场景 | 报错 | 解法 |
|------|------|------|
| 默认 scope=1，attach 非子进程 | `ptrace: Operation not permitted` | `echo 0 > /proc/sys/kernel/yama/ptrace_scope` 或用 root |
| 容器内 attach | 同上报错 | 加 `--cap-add=SYS_PTRACE` 或 `--privileged` |
| 目标进程设了 `PR_SET_DUMPABLE=0` | attach 被拒 | 需 root 或改程序（某些程序故意屏蔽调试） |

```bash
# 验证能否 ptrace：看 /proc/<pid>/status 的 TracerPid 或直接试
gdb -p $(pidof matching_engine)
# 报 Operation not permitted → 检查 ptrace_scope
```

## attach 后做什么：CPU 打满与卡死两类现场

### 场景 A：CPU 100%（死循环 / 忙等）

```c
// spin.c —— 某个线程死循环把 CPU 打满
#include <pthread.h>
volatile int counter = 0;
void *spin(void *a) { while (1) counter++; return NULL; }
int main(void) {
    pthread_t t; pthread_create(&t, NULL, spin, NULL);
    while (1) sleep(1); return 0;
}
```

```bash
top -H -p $(pidof spin)     # 先看是哪个 TID 打满 CPU（-H 按线程）
#   PID   TID  %CPU
#  12345 12346  99.9   ← 罪魁线程 TID=12346

gdb -p 12345
(gdb) info threads          # 找到 TID 12346 对应的 gdb 线程号
(gdb) thread 2
(gdb) bt
#0  spin () at spin.c:5
#1  ... in start_thread ()   # ← 钉到 spin() 第 5 行的 while(1)
(gdb) print counter          # 看计数是否异常
```

### 场景 B：进程「不动了」（阻塞在 syscall）

```bash
# 先看进程状态：S=睡眠(等IO/锁) D=不可中断睡眠(常是 IO)
ps -o pid,tid,stat,wchan,comm -L -p $(pidof matching_engine)
#  PID   TID STAT WCHAN          COMMAND
#  12345 12345 Ss   hrtimer_nanosleep  matching_engine
#  12345 12346 Sl   sk_wait_data       matching_engine  ← 卡在 socket recv

gdb -p 12345
(gdb) thread apply all bt
# Thread 2:
# #0  recvfrom () from libc
# #1  ... in md_recv_loop () at feed.c:88   ← 卡在等行情数据
```

`WCHAN` 列直接告诉你线程在内核里等什么：`sk_wait_data`=等 socket 数据、`hrtimer_nanosleep`=sleep、`futex_wait`=等锁。attach 前先 `ps -L` 看 WCHAN，往往已经能定位七成。

## 信号处理：attach 时 Ctrl+C 会怎样

attach 一个进程后按 Ctrl+C，gdb 默认**拦下 SIGINT 不传给程序**。信号处理策略：

```gdb
(gdb) handle SIGINT stop print nopass   # 收到 SIGINT：停下、打印、不传给程序（默认）
(gdb) handle SIGINT pass                # 把 SIGINT 传给程序（程序有自己的 handler 时）
(gdb) handle SIGPIPE nostop noprint pass  # 忽略某些信号
(gdb) info signals SIGINT               # 查看当前信号处理设置
```

> 交易进程常自己捕获 `SIGTERM`/`SIGINT` 做优雅退出。attach 调试时若 gdb 把信号全吞了，会干扰程序逻辑——用 `handle ... pass` 放行。

## fork / exec 跟随：调试会 fork 的进程

交易系统常有「主进程 + fork 子进程」或「exec 换壳」的结构。gdb 默认 attach 父进程后，子进程仍自由运行：

```gdb
(gdb) set follow-fork-mode parent    # 跟随父进程（默认）
(gdb) set follow-fork-mode child     # fork 后切到子进程调试
(gdb) set detach-on-fork on          # 不跟的那一侧继续跑（默认）
(gdb) set detach-on-fork off         # 两侧都留在 gdb 控制下（用 inferior 管理）

(gdb) catch fork       # 在 fork 调用处停下
(gdb) catch exec       # 在 exec 调用处停下
(gdb) info inferiors   # 查看 gdb 跟踪的所有进程（inferior）
(gdb) inferior 2       # 切换到第 2 个 inferior
```

| 命令 | 作用 |
|------|------|
| `set follow-fork-mode child` | fork 后跟进子进程 |
| `set detach-on-fork off` | 父子进程都跟踪（多 inferior） |
| `catch exec` | exec 换壳时停下，方便在新程序入口设断点 |
| `inferior N` | 在多进程间切换 |

## HFT 关联

1. **不重启抓现场**：行情网关「某个连接不动了」、下单引擎「延迟飙高」，`gdb -p` 上去 `thread apply all bt` + 看 WCHAN，几秒定位，不用冒着断流风险重启。
2. **ptrace 权限预配置**：交易机部署前把 `/proc/sys/kernel/yama/ptrace_scope` 设为 0（或给运维账号 CAP_SYS_PTRACE），否则事故发生时 attach 不上，只能干瞪眼。
3. **容器内调试**：跑在容器里的交易服务，Docker 必须带 `--cap-add=SYS_PTRACE`，否则 attach 一律 `Operation not permitted`。
4. **fork 模型跟子进程**：主控进程 fork 出 worker 时，`set follow-fork-mode child` + `set detach-on-fork off` 才能跟到 worker 内部。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** attach 的本质机制是什么？为什么 attach 后进程会暂停？

> attach 本质是 gdb 对目标进程发起 `ptrace(PTRACE_ATTACH)`，内核给目标发一个 SIGSTOP 使其暂停，之后 gdb 通过 `ptrace` 读写目标的内存、寄存器、控制单步。这就是为什么 attach 需要 ptrace 权限、且目标会「停住」。

**Q2:** `ptrace_scope=1` 时，运维用户 attach 一个已运行的交易进程会失败，为什么？怎么解决？

> scope=1 只允许**父子进程**间 ptrace，运维用户和交易进程没有父子关系，所以 `Operation not permitted`。解决：临时 `echo 0 > /proc/sys/kernel/yama/ptrace_scope`，或用 root（有 CAP_SYS_PTRACE），或让进程由调试者 fork 出来（保持父子关系）。

**Q3:** attach 后 `quit` 直接退出的风险是什么？

> gdb 会提示 active debugging session 并询问是否 kill 目标进程；若误确认，会把生产进程杀掉。正确姿势是**先 `detach`**（进程恢复运行、脱离控制）再 `quit`。

**Q4:** 进程「卡住」但 CPU 不高，怎么快速判断是「等锁」还是「等 IO」？

> 用 `ps -o stat,wchan -L -p <pid>` 看每个线程的 WCHAN：`futex_wait` 或 `futex_*` 通常是等锁（可能死锁）；`sk_wait_data`/`wait_woken` 等是等网络 IO；`hrtimer_nanosleep` 是 sleep。再 `gdb -p` attach 用 `thread apply all bt` 坐实具体卡在哪个函数。

**Q5:** 调试会 `fork` 的进程，为什么默认「跟不到子进程」？

> gdb 默认 `follow-fork-mode parent` 且 `detach-on-fork on`，fork 后 gdb 继续盯父进程，子进程被 detach 自由运行。要跟子进程，得 `set follow-fork-mode child`；要两个都盯，`set detach-on-fork off` 后用 `inferior` 管理多进程。

</details>

## 交叉引用

- [2.1 多线程调试](01-thread-debugging.md)
- [2.3 rr 可逆调试](03-rr-reversible-debugging.md)
- [1.1 gdb 入门与调试信息](../../chapter-01-gdb-basics/notes/01-gdb-intro-build.md)
- [03.6 模块导读](../../README.md)
