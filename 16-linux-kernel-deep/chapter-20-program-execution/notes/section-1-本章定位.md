## 1. 本章定位

> **ULK Ch 20 Program Execution** · 磁盘上的 **静态文件** → 内存中的 **动态进程**

---

### 一、本章讲什么

「加载并跳转」在现代 OS 中高度复杂：

| 主题 | 要点 |
|------|------|
| **凭证 / 能力** | UID/GID、capabilities、SELinux 钩子 |
| **内存布局** | text/data/BSS/stack、argv/envp、灵活 mmap 布局 |
| **ptrace** | gdb 底层 — 单步 TF 标志 |
| **格式** | ELF、`#!`、binfmt_misc、personality |
| **`execve`** | `do_execve` — 换 mm、映射 ELF、改 EIP/ESP |

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-进程凭证与能力.md) | real/effective UID、CAP_*、security_ops |
| [3](./section-3-地址空间与内存布局.md) | 段、栈上 argv/envp、RLIMIT_STACK 灵活布局 |
| [4](./section-4-执行跟踪ptrace.md) | PTRACE_ATTACH、SINGLESTEP |
| [5](./section-5-可执行格式与执行域.md) | ELF、binfmt_misc、personality |
| [6](./section-6-execve与全书索引.md) | do_execve、start_thread、ULK 全书索引 |

---

### 三、在 Linux 链上的位置

```
Ch 3  fork → 子进程
Ch 9  mm_struct、VMA、mmap
Ch 10 execve syscall
Ch 20 加载 ELF、设入口（本章）
Ch 7  新程序被调度运行
01 CSAPP Ch 8  链接与加载（用户态视角）
```

HFT：理解 **exec 不返回**、**动态链接器**、**ASLR/布局** 对 **延迟与确定性** 的影响；生产服务少用 ptrace。

---

← [Ch 20 导读](../README.md) · 下一节 [2. 凭证](./section-2-进程凭证与能力.md)
