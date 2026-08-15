# TLPI 第 27 章 — Program Execution

**优先级**：🔴（shell、服务拉起外部程序、fork+exec 标准模型）  
**前置**：[Ch24 fork](../chapter-24-process-creation/README.md) · [Ch25 终止](../chapter-25-process-termination/README.md) · [Ch26 wait](../chapter-26-monitoring-child-processes/README.md)  
**后置**：[Ch28 fork/exec 细节](../chapter-28-process-creation-exec-detail/README.md) · [Ch9 凭证 / SUID](../chapter-09-process-credentials/README.md)（SUID 在 exec 时生效）

---

## 小节目录

- [27.1 六兄弟](notes/27.1-executing-a-new-program-execve.md)
- [27.2 保留 vs 销毁](notes/27.2-the-exec-library-functions.md)
- [27.3 `FD_CLOEXEC` / `O_CLOEXEC`](notes/27.3-interpreter-scripts.md)
- [27.4 PATH（`*p`）](notes/27.4-file-descriptors-and-exec.md)
- [27.5 shebang `#!`](notes/27.5-signals-and-exec.md)
- [27.7 环境变量](notes/27.7-implementing-system.md)

---

## 章节目标


掌握 exec 六兄弟与 `execve`；理清保留/销毁资源；`FD_CLOEXEC` / `O_CLOEXEC`；PATH 与 shebang；熟练 **fork + exec + waitpid**。

---


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


---

## 练习 / 实验清单


1. `execvp` / `execl`  
2. `FD_CLOEXEC` 跨 exec  
3. （选）shebang  
4. fork+exec+重定向模板  
5. （选）exec 前后 handler 对比  

---


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


---

## 参考


- Kerrisk · TLPI Ch27  
- `man 3 exec` · `man 2 execve` · `man 2 fcntl`（`FD_CLOEXEC`）


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

/* Ch27 程序执行 — execl/execlp/execv/execvp。
 * exec 替换当前进程映像，pid 不变。
 * 编译: gcc -o ch27_demo ch27_demo.c */

int main(void) {
    /* 方式1: execlp — 搜索 PATH，变长参数 */
    pid_t pid = fork();
    if (pid == 0) {
        printf("Child: calling execlp(\"ls\")\n");
        execlp("ls", "ls", "-l", "/tmp", NULL);
        perror("execlp");  /* 只有失败才返回 */
        _exit(1);
    }
    waitpid(pid, NULL, 0);

    /* 方式2: execvp — 搜索 PATH，数组参数 */
    pid = fork();
    if (pid == 0) {
        char *args[] = {"echo", "hello from execvp", NULL};
        printf("Child: calling execvp(\"echo\")\n");
        execvp("echo", args);
        perror("execvp");
        _exit(1);
    }
    waitpid(pid, NULL, 0);

    /* 方式3: execv — 完整路径，数组参数 */
    pid = fork();
    if (pid == 0) {
        char *args[] = {"/bin/date", NULL};
        printf("Child: calling execv(\"/bin/date\")\n");
        execv("/bin/date", args);
        perror("execv");
        _exit(1);
    }
    waitpid(pid, NULL, 0);

    /* 传递环境变量给 exec */
    pid = fork();
    if (pid == 0) {
        char *args[] = {"env", NULL};
        char *envp[] = {"MY_VAR=hello_from_parent", NULL};
        printf("Child: calling execve with custom env\n");
        execve("/usr/bin/env", args, envp);
        perror("execve");
        _exit(1);
    }
    waitpid(pid, NULL, 0);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
