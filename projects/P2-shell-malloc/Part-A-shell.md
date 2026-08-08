# Part A — Mini Shell 实现指南

> 从零写一个能跑管道、重定向、信号的 shell。每一步都能编译运行，每一步都有可见的反馈。

## 你在做什么

shell 的本质就是一个**死循环**：读一行 → 拆成命令 → fork 子进程 → exec 替换成目标程序 → wait 等它结束。

```
┌─────────────────────────────────────────────────┐
│  while (1) {                                     │
│    print prompt → 读一行 → 拆 token              │
│    ↓                                             │
│    是内置命令(cd/exit/pwd)? → 直接执行            │
│    ↓ 否                                          │
│    fork() ──→ 子进程: execvp(命令)                │
│           ──→ 父进程: waitpid(子进程)             │
│  }                                               │
└─────────────────────────────────────────────────┘
```

就这么多。CSAPP Ch8 讲的 fork/exec/wait 三件套，全部浓缩在这一张图里。

## 文件结构

```
P2-shell-malloc/
├── shell/
│   ├── Makefile
│   ├── shell.c          ← 主循环 + 内置命令
│   ├── parser.c         ← 词法分析（拆 token）
│   ├── parser.h
│   ├── executor.c       ← fork/exec/pipe/redirect
│   ├── executor.h
│   ├── builtin.c        ← cd / exit / pwd
│   └── builtin.h
```

分文件不是装样子——拆开后你能单独测每个模块，parser 出错不影响 executor。

---

## Phase 1：能跑 `ls`（30 分钟）

**目标：** 输入 `ls`，屏幕列出文件。这就是你的第一个 shell。

### 关键 API

```c
#include <unistd.h>    // fork, execvp
#include <sys/wait.h>  // waitpid

// fork() 返回两次：子进程返回 0，父进程返回子进程 PID
pid_t fork(void);

// execvp 用 PATH 搜索命令，替换当前进程的代码+数据
// 成功不返回（进程已经被替换），失败返回 -1
int execvp(const char *file, char *const argv[]);

// 等子进程结束
pid_t waitpid(pid_t pid, int *status, int options);
```

### 代码骨架

```c
// shell.c — Phase 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 1024

int main(void) {
    char line[MAXLINE];

    while (1) {
        printf("mysh> ");              // 打印提示符
        fflush(stdout);                // 必须刷新，否则不显示
        if (!fgets(line, sizeof(line), stdin))
            break;                     // Ctrl-D 退出

        // 去掉换行符
        line[strcspn(line, "\n")] = '\0';

        // 简单拆分：按空格切成 token
        char *argv[64];
        int argc = 0;
        char *tok = strtok(line, " ");
        while (tok && argc < 63) {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
        argv[argc] = NULL;             // execvp 要求 NULL 结尾

        if (argc == 0) continue;       // 空行

        // fork + exec
        pid_t pid = fork();
        if (pid == 0) {
            // 子进程
            execvp(argv[0], argv);
            // 只有 execvp 失败才会走到这里
            perror("execvp");
            exit(127);
        } else if (pid > 0) {
            // 父进程：等子进程结束
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }
    return 0;
}
```

### 编译运行

```bash
gcc -Wall -Wextra -g shell.c -o shell
./shell
mysh> ls
mysh> echo hello
mysh> cat /etc/hostname
```

### 常见坑

| 坑 | 原因 | 解决 |
|----|------|------|
| 提示符不显示 | stdout 是行缓冲，printf 没到换行 | `fflush(stdout)` |
| execvp 成功了但没输出 | 不会发生——成功就替换进程了 | 如果走到 perror 说明 exec 失败了 |
| Ctrl-D 不退出 | fgets 返回 NULL 时没 break | 检查 fgets 返回值 |
| 子进程变僵尸 | 没调 waitpid | fork 后父进程必须 waitpid |

### 卡住翻哪篇笔记

| 问题 | 翻哪 |
|------|------|
| fork 返回值搞不清 | CSAPP 8.4 进程控制 → fork |
| execvp 和 execve 区别 | CSAPP 8.4 → execve 系列 |
| waitpid 的 status 怎么用 | CSAPP 8.4 → waitpid + WIFEXITED 宏 |

---

## Phase 2：内置命令 cd / exit / pwd（20 分钟）

**目标：** `cd /tmp` 能切换目录，`pwd` 显示当前目录，`exit` 退出 shell。

### 为什么 cd 必须是内置命令

`cd` 改的是**当前进程的工作目录**（chdir 系统调用）。如果 fork 子进程来执行 cd，改的是子进程的目录，子进程一退出就没了，父进程（shell 本身）的目录没变。所以 cd **必须在 shell 自己的进程里执行**。

```c
// builtin.c
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// 返回 1 表示是内置命令已处理，0 表示不是
int try_builtin(char **argv, int argc) {
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = argc > 1 ? argv[1] : getenv("HOME");
        if (chdir(dir) != 0)
            perror("cd");
        return 1;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char buf[4096];
        if (getcwd(buf, sizeof(buf)))
            printf("%s\n", buf);
        return 1;
    }
    return 0;  // 不是内置命令
}
```

### 改主循环

```c
// shell.c 主循环里，fork 之前加一行：
if (try_builtin(argv, argc))
    continue;  // 内置命令已处理，跳过 fork/exec
```

### 测试

```bash
mysh> pwd           # 显示当前目录
mysh> cd /tmp
mysh> pwd           # 应该显示 /tmp
mysh> exit          # 退出
```

---

## Phase 3：I/O 重定向 `>` 和 `<`（30 分钟）

**目标：** `ls > out.txt` 把输出写入文件，`wc < out.txt` 从文件读输入。

### 原理

Linux 的文件描述符表是一个数组。`dup2(oldfd, newfd)` 做的事是：把 newfd 指向 oldfd 指向的文件表项。简单说：

```
dup2(fd, STDOUT_FILENO)  →  以后所有 printf/write(1, ...) 都写到 fd 指向的文件
dup2(fd, STDIN_FILENO)   →  以后所有 scanf/read(0, ...) 都从 fd 指向的文件读
```

**必须在 fork 之后、exec 之前调用 dup2**——因为 dup2 改的是子进程的 fd 表，exec 保留 fd 表。

### 代码骨架

```c
// executor.c
#include <fcntl.h>
#include <unistd.h>

void execute(char **argv, int redirect_in, char *infile,
                        int redirect_out, char *outfile) {
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程：设置重定向
        if (redirect_in) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (redirect_out) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(argv[0], argv);
        perror("execvp");
        exit(127);
    }
    // 父进程
    int status;
    waitpid(pid, &status, 0);
}
```

### 解析器要改什么

在拆 token 时检测 `>` 和 `<`：
- 遇到 `>`：标记 redirect_out=1，下一个 token 是文件名，不放入 argv
- 遇到 `<`：标记 redirect_in=1，下一个 token 是文件名，不放入 argv

### 卡住翻哪篇笔记

| 问题 | 翻哪 |
|------|------|
| dup2 到底干了什么 | CSAPP 10.9 I/O 重定向 |
| fd 表和文件表的关系 | CSAPP 10.8 共享文件 |
| 为什么 fork 后 dup2 | CSAPP 8.4 → fork 复制 fd 表 |

---

## Phase 4：管道 `|`（1 小时）

**目标：** `ls | grep .c | wc -l` 多级管道能跑。

### 原理

`pipe(fds)` 创建一对 fd：`fds[0]` 是读端，`fds[1]` 是写端。数据从写端流入，读端流出。

多级管道 `a | b | c` 的接线方式：

```
a 的 stdout ──→ pipe1[1]    pipe1[0] ──→ b 的 stdin
b 的 stdout ──→ pipe2[1]    pipe2[0] ──→ c 的 stdin
c 的 stdout ──→ 终端
```

每个命令都是 fork 出来的子进程，管道在 fork 之前创建。

### 代码骨架

```c
// executor.c — 多级管道
// commands 是一个二维数组：commands[0..ncmds-1]
// 每个 commands[i] 是一个 argv 数组

void execute_pipeline(char ***commands, int ncmds) {
    int prev_read = -1;  // 上一条命令的读端（给下一条当 stdin）

    for (int i = 0; i < ncmds; i++) {
        int pipefd[2];
        // 最后一条命令不需要创建管道（输出到终端）
        if (i < ncmds - 1) {
            pipe(pipefd);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // 子进程
            // stdin 接上一条管道的读端
            if (prev_read != -1) {
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            // stdout 接当前管道的写端
            if (i < ncmds - 1) {
                close(pipefd[0]);              // 子进程不需要读端
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            execvp(commands[i][0], commands[i]);
            perror("execvp");
            exit(127);
        }
        // 父进程
        if (prev_read != -1) close(prev_read);  // 关掉上一条的读端
        if (i < ncmds - 1) {
            close(pipefd[1]);                    // 父进程不需要写端
            prev_read = pipefd[0];               // 记住读端给下一条用
        }
    }

    // 等所有子进程结束
    for (int i = 0; i < ncmds; i++) {
        int status;
        wait(&status);
    }
}
```

### 最常见的坑：管道卡死

**症状：** `ls | grep x` 永远不返回，shell 卡住。

**原因：** 父进程没关管道的写端。grep 一直在等更多输入（因为还有进程持有写端），永远不会收到 EOF。

**解决：** 父进程创建管道后，fork 后**立即关闭不需要的端**。规则：
- 父进程关掉所有 pipefd[1]（写端）
- 父进程只保留最后一个 pipefd[0] 给下一条命令用
- 每条子进程关掉不属于自己的管道端

### 解析器要改什么

在拆 token 时遇到 `|`，把当前命令存入 commands 数组，开始新命令。最终 commands 是一个命令数组。

### 测试

```bash
mysh> ls | grep .c
mysh> ls | wc -l
mysh> echo hello | cat | cat | wc -c    # 6 (hello\n)
mysh> cat /etc/passwd | grep root | wc -l
```

### 卡住翻哪篇笔记

| 问题 | 翻哪 |
|------|------|
| pipe 返回的 fd 怎么用 | CSAPP 8.4 → pipe |
| 管道为什么卡死 | 父进程没关写端 → grep 读不到 EOF |
| fd 泄漏导致管道卡死 | 每次 fork 后检查哪些 fd 该关没关 |

---

## Phase 5：后台 `&` 和信号 Ctrl-C（40 分钟）

**目标：** `sleep 10 &` 后台运行不阻塞 shell；Ctrl-C 只杀前台子进程，不杀 shell。

### 后台命令

在解析时检测行尾的 `&`。如果是后台命令，父进程 fork 后**不调 waitpid**，直接继续读下一行。

```c
if (background) {
    // 不等待，直接继续
    // 可选：打印子进程 PID
    printf("[bg] pid=%d\n", pid);
} else {
    waitpid(pid, &status, 0);
}
```

后台子进程结束后会变僵尸，需要在某个时刻回收。最简单的做法：用 `SIGCHLD` 信号处理函数异步回收。

### 信号处理

shell 启动时注册 SIGINT（Ctrl-C）处理器：

```c
#include <signal.h>

void sigint_handler(int sig) {
    // 什么都不做，只是不让 Ctrl-C 杀掉 shell
    // 写一个换行让提示符好看
    write(STDOUT_FILENO, "\n", 1);
}

int main(void) {
    signal(SIGINT, sigint_handler);  // 注册 SIGINT 处理器
    // ...
}
```

**关键理解：** fork 后子进程会继承父进程的信号处理器。所以子进程（execvp 之前）要恢复 SIGINT 为默认行为：

```c
if (pid == 0) {
    signal(SIGINT, SIG_DFL);  // 子进程：Ctrl-C 直接杀
    execvp(argv[0], argv);
}
```

### SIGCHLD 回收僵尸

```c
void sigchld_handler(int sig) {
    // 非阻塞回收所有已结束的子进程
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(void) {
    signal(SIGCHLD, sigchld_handler);
    // ...
}
```

### 测试

```bash
mysh> sleep 5 &          # 立刻返回，可以继续输命令
mysh> ls                 # 正常工作
mysh> sleep 10           # 等待中按 Ctrl-C
^C
mysh>                    # shell 还活着，子进程被杀
```

### 卡住翻哪篇笔记

| 问题 | 翻哪 |
|------|------|
| signal 和 sigaction 区别 | CSAPP 8.5 信号 |
| 僵尸进程怎么产生的 | CSAPP 8.4 → waitpid 不调就僵尸 |
| SIGCHLD 为什么要 WNOHANG | 非阻塞回收，不能在信号处理器里阻塞 |
| 信号处理器里能调 printf 吗 | 不能！printf 不是异步信号安全函数。用 write |

---

## 完成检查清单

- [ ] `ls` / `echo` / `cat` 能跑
- [ ] `cd /tmp && pwd` 正确
- [ ] `echo hello > out.txt` 文件内容正确
- [ ] `wc < out.txt` 从文件读
- [ ] `ls | grep .c | wc -l` 三级管道
- [ ] `sleep 3 &` 后台不阻塞
- [ ] Ctrl-C 不杀 shell，只杀前台命令
- [ ] Ctrl-D 优雅退出

## 学完你应该能回答

1. fork 为什么返回两次？父子进程各自看到什么返回值？
2. execvp 成功后原进程的代码还在吗？fd 表呢？
3. 管道 `a | b` 里，b 的 stdin 是怎么变成 a 的 stdout 的？画出 fd 表的变化。
4. 为什么 cd 必须是内置命令，不能 fork 执行？
5. Ctrl-C 为什么只杀前台进程不杀 shell？信号处理器在哪里设的？
