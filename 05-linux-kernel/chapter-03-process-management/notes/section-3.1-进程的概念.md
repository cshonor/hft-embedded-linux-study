## ① 进程的概念 · The Process

**进程** = 处于 **执行期** 的程序 + **相关资源** 的集合。程序本身是磁盘上的静态文件；进程是 **加载后、正在跑（或曾跑过）** 的动态实体。

| 资源示例 | 说明 |
|----------|------|
| 打开的文件 | fd 表、当前工作目录 |
| 挂起的信号 | 待递送 / 已屏蔽信号集 |
| 处理器状态 | 寄存器、PC、栈指针 |
| 内存地址空间 | 代码 / 数据 / 堆 / 栈 / mmap |
| 一个或多个执行线程 | Linux 中仍属「进程」模型 |
| 凭证 / 命名空间 | UID、PID 命名空间等（扩展） |

#### 进程 vs 程序

| 维度 | 程序 | 进程 |
|------|------|------|
| 形态 | 磁盘文件（ELF 等） | 内核 `task_struct` + 地址空间 |
| 数量 | 一份二进制可对应 **多个** 进程 | 每个实例独立 PID |
| 生命周期 | 持久 | 创建 → 运行 → 退出 |

#### 典型创建路径（Unix 两步）

```
fork()  ──► 复制现有进程（子进程）
   │
   └──► exec() 族 ──► 加载新可执行文件，替换映像
```

| 调用 | 作用 | 返回值语义 |
|------|------|------------|
| **`fork()`** | 复制进程 — 父子并发 | 父得子 PID，子得 0 |
| **`exec*()`** | 换程序 — 常接在 `fork` 之后 | 成功 **不返回**（同一 PID，新映像） |
| **`wait()` / `waitpid()`** | 父进程回收子退出状态 | 见 §3.6 |

#### fork 语义：一次调用，返回两次

> 易混点：fork 复制的是 **进程**，不是线程。产物是一个几乎完整的进程拷贝（COW），
> 不是"自动产生的线程B"。子进程诞生那一刻就停在 fork() 的返回处，接着往下执行
> **同一段代码**——所以看起来"没显式调用就出现了"，实际是地址空间整体被复制。
> 一次调用、两个返回点：父进程拿到子 PID，子进程拿到 0，靠返回值区分各自身份。
>
> 多线程进程里调 fork：**只有调用 fork 的那个线程被复制**，其余线程直接消失（POSIX
> 规定）。若消失的线程持有锁，子进程里该锁永远无法解锁——多线程程序慎用 fork。

#### 最小示例（带参数注释）

```c
pid_t pid = fork();          /* 系统调用：复制当前进程。父返回子PID，子返回 0 */
if (pid == 0) {
    /* ---- 子进程分支 ---- */
    execl("/bin/ls",        /* 参数1: 新程序的路径（要加载的可执行文件） */
          "ls",             /* 参数2: argv[0] —— 惯例写程序名，程序自己标识自己 */
          "-l",             /* 参数3: argv[1] —— 传给 ls 的选项，可继续追加 argv[2]... */
          NULL);            /* 末尾: 哨兵，标记变长参数列表结束，漏写=未定义行为 */
    _exit(127);             /* exec 成功永不返回（映像已被替换）；到这里=exec 失败 */
}
/* ---- 父进程分支 ---- */
waitpid(pid,                /* 参数1: 要等的子进程 PID（wait() = 等任意子进程） */
        &status,            /* 参数2: 传出参数，内核把子进程退出码写到这里 */
        0);                 /* 参数3: 选项位。0 = 阻塞等到它退出为止 */
                             /* 不 wait 的后果：子进程退出后变僵尸（Z），task_struct 无法释放 */
```

#### exec 族速记与调用示例

后缀是正交的开关，任意组合：**函数名 = `exec` + 最多 2 个后缀（l/v 二选一，p/e 可选）**，共 6 个。

| 函数 | 后缀拆解 | 调用示例 |
|------|----------|----------|
| `execl` | l = list | `execl("/bin/ls", "ls", "-l", NULL);` |
| `execlp` | l + p = 搜 PATH | `execlp("ls", "ls", "-l", NULL);` ← 路径可省 |
| `execle` | l + e = 自带 envp | `execle("/bin/ls", "ls", NULL, myenvp);` ← 变参表的 NULL 后面跟 envp |
| `execv` | v = vector | `char *av[] = { "ls", "-l", NULL }; execv("/bin/ls", av);` |
| `execvp` | v + p | `execvp("ls", av);` |
| `execve` | v + e（**唯一系统调用**） | `execve("/bin/ls", av, myenvp);` |

| 后缀 | 含义 |
|------|------|
| `l` | list —— 参数逐个列出，NULL 结尾 |
| `v` | vector —— 参数打包成 `char *argv[]` 数组 |
| `p` | 搜 `PATH` 环境变量找程序（否则必须写全路径） |
| `e` | 自带环境变量数组 `envp`（否则继承当前环境） |

> `execve` / `execveat` 是真正的系统调用，其余 execl/execv/execlp... 都是 libc 包装。

#### exec 独立示例：程序 exec 自己，验证「PID 不变、映像已换」

```c
/* exec_demo.c —— gcc -Wall -o exec_demo exec_demo.c && ./exec_demo */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("[run] PID=%ld  argv[0]=%s  argc=%d\n",
           (long)getpid(), argv[0], argc);

    if (argc == 1) {                       /* 第一轮：没带额外参数 */
        char heap_var = 'A';               /* 堆/栈/全局变量 —— exec 后全部蒸发 */
        char *newargv[] = { "fake-name",   /* argv[0] 可以随便骗：ps 里显示这个名字 */
                            "stage2", NULL };
        printf("heap_var=%c 即将被销毁\n", heap_var);
        fflush(stdout);                    /* stdio 缓冲区也是旧映像的，exec 前必须冲刷 */
        execv("/proc/self/exe", newargv);  /* /proc/self/exe = 内核记录的本程序真实路径 */
        perror("execv");                   /* exec 返回 = 失败，且只有失败才返回 */
        return 127;                        /* 惯例退出码 127 = command not found */
    }
    /* 第二轮：已被新映像替换，从这里重新开始 */
    printf("[new image] heap_var 已不存在；argv 被换成 \"%s\" \"%s\"\n",
           argv[0], argv[1]);
    return 0;
}
```

运行输出：

```
[run] PID=22841  argv[0]=./exec_demo  argc=1
heap_var=A 即将被销毁
[new image] heap_var 已不存在；argv 被换成 "fake-name" "stage2"
```

两轮 PID 相同 → **还是同一个进程**；argv/内存全变 → **程序映像已被整个替换**。
用 `strace ./exec_demo` 可以直接看到那条系统调用：`execve("/proc/self/exe", ["fake-name", "stage2"], /* environ */) = 0`。

#### 什么能跨过 exec 存活

| 存活 ✔ | 说明 |
|--------|------|
| PID / PPID | 进程还是那个进程 |
| 打开的 fd（未设 `O_CLOEXEC`） | 继承——shell 重定向就靠这个 |
| 当前工作目录、umask、nice | 属性不随映像销毁 |
| 信号屏蔽字 | 保留；但 **已捕获的信号处理函数重置为默认**（新映像没有旧 handler） |

| 丢失 ✘ | 说明 |
|--------|------|
| 代码 / 数据 / 堆 / 栈 | 整个用户地址空间推倒重建（§3.7） |
| 全局/局部变量、`atexit` 处理器 | 随旧映像消失 |
| 环境变量 | 默认继承 environ，但 `execve`/`execle` 传新 `envp` 时整个替换 |

#### 内核视角（预告 §3.2）

```
用户态进程                内核
┌─────────────┐          ┌──────────────┐
│  代码/堆/栈   │ ◄─mm──► │ task_struct  │
│  系统调用     │ ──────► │ 调度/文件/信号 │
└─────────────┘          └──────────────┘
```

| 用户态工具 | 看到的「进程」 |
|------------|----------------|
| `ps` / `top` | PID、STAT、CPU%、命令行 |
| `/proc/<pid>/` | 内核导出只读视图 |
| `strace` | 系统调用轨迹 |

**HFT：** 热路径网关多用 **线程池 + `clone`/`pthread`**，极少 **每连接 `fork`**（页表/COW 与调度切换仍有成本）。Shell 脚本、配置守护、隔离沙箱才常见 `fork`+`exec`；行情/发单进程通常是 **长寿命单进程多线程**。

→ [§3.2 task_struct](./section-3.2-进程描述符与任务结构.md) · [§3.4 fork/COW](./section-3.4-进程创建与写时拷贝.md) · [07 TLPI Ch24 进程创建](../../../03-linux-userspace-api/chapter-24-process-creation/notes) · [01 CSAPP Ch8 fork/exec](../../../02-computer-systems/chapter-08-exceptional-control-flow/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 进程和程序的根本区别是什么？fork() 后父子进程共享什么？

<details><summary>答案</summary>

程序 = 磁盘上的静态可执行文件；进程 = 程序加载到内存后的动态执行实例 + 资源（页表/fd/信号/栈）。fork() 后父子共享：代码段（只读）、打开的文件描述符（共享 file 结构）、内存映射（COW 模式下页表项标记只读，写时才拷贝）。

</details>

**Q2.** HFT 交易系统为什么通常用多进程而非多线程？

<details><summary>答案</summary>

多进程：一个 crash 不影响另一个（进程隔离）；但 IPC 开销大。多线程：共享内存通信零拷贝；但一个线程 crash 整个进程挂。HFT 选择取决于：策略进程用多进程隔离（一个策略 crash 不影响风控），行情解码用多线程共享内存（零拷贝传递行情数据）。

</details>

**Q3.** 含 4 个线程的进程，其中线程A 调用 fork()，子进程里有几个线程？execl 执行成功后，进程的 PID 变吗？

<details><summary>答案</summary>

子进程只有 **1 个线程**（线程A 的拷贝），其余 3 个线程不复制——这是 POSIX 规定，也是多线程程序用 fork 的经典坑（消失线程持有的锁永远锁死）。execl 成功后 PID **不变**：exec 只替换程序映像（代码/数据/堆栈重载），进程还是那个进程。

</details>

</details>
---
