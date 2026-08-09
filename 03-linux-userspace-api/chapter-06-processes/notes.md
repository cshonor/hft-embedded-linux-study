# TLPI 第 06 章 — Processes

> 对应目录：`chapter-06-processes/`  
> 书名原文：**Processes**  
> ⚠️ **本章不讲 `fork`/`exec`**，只打地基；创建/退出/等待/加载程序在 **Ch24–Ch27**。

**优先级**：🔴（后续 fork/信号/多进程的地基）  
**前置**：[Ch3](../chapter-03-system-programming-concepts/notes.md) · [Ch4](../chapter-04-file-io-universal/notes.md) · [Ch5](../chapter-05-file-io-further/notes.md)  
**后置**：[Ch7 内存分配](../chapter-07-memory-allocation/notes.md) · [Ch8 用户与组](../chapter-08-users-and-groups/notes.md) · [Ch24 fork](../chapter-24-process-creation/notes.md)  
**内核对照**：[LKD Ch3 进程管理](../../05-linux-kernel/00_Book_3rd_Notes/chapter-03-process-management/) · CSAPP Ch8/Ch9

---

## 章节目标

建立进程基础模型：虚拟地址空间布局、命令行参数、环境变量、PID/PPID；掌握非局部跳转 `setjmp`/`longjmp`，为后续 fork、exec、信号、多进程铺路。

---

## 6.1 进程基本概念

| | |
|--|--|
| **程序（program）** | 磁盘上的静态可执行文件 |
| **进程（process）** | 加载进内存、正在执行的**动态实例** |

进程 = 内核管理的执行上下文；内核为其分配资源以运行程序。

内核视角两大块：

1. **用户空间内存**：代码、数据、栈、堆等  
2. **内核 PCB**（Linux：`task_struct`）：状态、PID、fd 表、信号掩码、资源限制等  

---

## 6.2 PID 与 PPID

```c
#include <unistd.h>
pid_t getpid(void);   /* 当前 PID；永不失败 */
pid_t getppid(void);  /* 父进程 PID */
```

| 点 | |
|----|--|
| PID | 正整数；历史上常见上限 32767，见 `/proc/sys/kernel/pid_max`（现代可更大） |
| PID 1 | `init` / `systemd`，系统初代进程 |
| **孤儿进程** | 父先退出 → 被 PID 1 收养 → `getppid()` 常为 1 |
| 错误处理 | `getpid`/`getppid` **不会失败**，不必查 `-1` |

---

## 6.3 进程虚拟地址空间（本章核心）

用户空间（低 → 高，示意）：

```
高地址  ┌─────────────────────┐
        │  内核空间（用户不可访） │
        ├─────────────────────┤
        │  栈 Stack ↓         │  局部变量、参数、返回地址；ulimit -s
        ├─────────────────────┤
        │  mmap / 共享库 …    │
        ├─────────────────────┤
        │  堆 Heap ↑          │  malloc；边界 = program break（Ch7）
        ├─────────────────────┤
        │  BSS（未初始化数据）  │  加载时清零；磁盘映像几乎不占空间
        ├─────────────────────┤
        │  Data（已初始化数据） │  显式初始化的全局/静态
        ├─────────────────────┤
低地址  │  Text（代码段，只读） │  指令；同程序多进程可共享
        └─────────────────────┘
```

### 变量落点示例

```c
const char *s = "hello";  /* s：数据段；字面量：只读（常在 text/rodata） */
static int x;             /* BSS */
static int y = 5;         /* Data */
int main(void) {
    int z;                /* 栈 */
    int *p = malloc(4);   /* p 在栈；*p 在堆 */
}
```

Demo：[`code/mem_segments.c`](./code/mem_segments.c)

---

## 6.4 命令行参数 `argv`

```c
int main(int argc, char *argv[])
```

| | |
|--|--|
| `argc` | 参数个数 |
| `argv[0]` | 程序名（可为任意字符串；POSIX 允许与真实路径不一致） |
| `argv[argc]` | `NULL` |
| 来源 | 父进程 `fork`+`exec` 传入 |
| 可改性 | 可改字符串内容；勿随意改 `argv` 指针数组语义 |

---

## 6.5 环境变量

```c
extern char **environ;   /* NAME=VALUE 字符串数组，NULL 结尾 */
```

### API

```c
#include <stdlib.h>
char *getenv(const char *name);
int putenv(char *string);                                      /* 持有传入缓冲，慎用 */
int setenv(const char *name, const char *value, int overwrite); /* 推荐 */
int unsetenv(const char *name);
int clearenv(void);
```

| 要点 | |
|------|--|
| 继承 | 子进程默认继承父环境（`fork` 复制） |
| 作用域 | 进程内修改只影响**自己**及之后创建的子进程；**不影响**父进程/外层 shell |
| `setenv` vs `putenv` | `setenv` 复制字符串；`putenv` 直接用传入缓冲 → 局部数组会变野指针 |
| `getenv` | 返回指向环境区的指针；**不要 `free`** |
| `main` 第三参 `envp` | 非标准扩展，不推荐；用 `environ` |

典型用途：`PATH`、`LC_*`、`LD_LIBRARY_PATH`、配置传递。

Demo：[`code/t_getenv.c`](./code/t_getenv.c)

---

## 6.6 非局部跳转 `setjmp` / `longjmp`

```c
#include <setjmp.h>
int setjmp(jmp_buf env);           /* 直接调用返回 0；保存栈上下文 */
void longjmp(jmp_buf env, int val); /* 回到 setjmp；表现为返回 val（0→1） */
```

- 普通 `goto`：函数内。  
- `setjmp`/`longjmp`：**跨函数**非局部跳转。

### 易踩坑：优化变量

`setjmp` 与 `longjmp` 之间被修改的局部变量，若放在寄存器里，恢复后可能「回滚」：

```c
volatile int flag;  /* 强制走内存，禁止只放寄存器 */
```

典型场景：深层错误跳出、简易异常路径（信号处理里的用法见 Ch21，限制更多）。

Demo：[`code/setjmp_vars.c`](./code/setjmp_vars.c)

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

## 章节链路

```
Ch5  fd / 打开描述（stdin/out/err 已在进程里）
  → Ch6  进程模型、地址空间、环境、setjmp
  → Ch7  堆 / brk / malloc
  → Ch8  UID/GID
  → Ch24 fork：复制地址空间、环境、fd 表（本章直接落地）
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 搞清栈/堆/BSS；环境变量做配置；少依赖 `putenv` |
| HFT | 地址空间与后续 `mmap`/大页衔接；`setjmp` 少用在热路径 |

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

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 6 — Processes**  
- [OUTLINE](../OUTLINE.md) · [Ch5](../chapter-05-file-io-further/notes.md) · [Ch7](../chapter-07-memory-allocation/notes.md) · [Ch24](../chapter-24-process-creation/notes.md)
