# TLPI 第 27 章 — Program Execution

**优先级**：🔴（shell、服务拉起外部程序、fork+exec 标准模型）  
**前置**：[Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch25 终止](../chapter-25-process-termination/notes.md) · [Ch26 wait](../chapter-26-monitoring-child-processes/notes.md)  
**后置**：[Ch28 fork/exec 细节](../chapter-28-process-creation-exec-detail/notes.md) · [Ch9 凭证 / SUID](../chapter-09-process-credentials/notes.md)（SUID 在 exec 时生效）

---

## 小节目录

- [27.1 六兄弟](./notes/27.1-section-27-1.md)
- [27.2 保留 vs 销毁](./notes/27.2-section-27-2.md)
- [27.3 `FD_CLOEXEC` / `O_CLOEXEC`](./notes/27.3-fdcloexec-ocloexec.md)
- [27.4 PATH（`*p`）](./notes/27.4-path.md)
- [27.5 shebang `#!`](./notes/27.5-shebang.md)
- [27.7 环境变量](./notes/27.7-environment.md)

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

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
