## 6. `execve` 与全书索引

---

### 一、从 libc 到内核

所有 **`exec*`**（`execl`、`execvp` …）最终 → **`execve()`** → **`sys_execve()`** → **`do_execve()`**。

→ API vs syscall：[Ch 10 section-2](../chapter-10-system-calls/notes/section-2-POSIX-API与系统调用.md)

---

### 二、`do_execve()` 三步「脱胎换骨」

```
① 清理旧环境
   检查可执行权限 → 新 mm_struct → 释放旧地址空间/页表
        ↓
② 加载新程序
   search_binary_handler() → ELF loader
   映射 text/data/BSS、解释器、建立 VMA
        ↓
③ 篡改硬件上下文
   start_thread()：改内核栈上用户态寄存器
   EIP → 程序入口（或 ld.so）
   ESP → 新用户栈顶（argv/envp 已压栈）
        ↓
execve「返回」用户态 → CPU 执行新程序第一条指令
```

**成功时 `execve` 永不返回** 调用者 — 原程序地址空间已销毁；**失败** 才返回 -1。

→ fork 后 exec：[Ch 3 section-6](../chapter-03-processes/notes/section-6-创建与销毁.md)

---

### 三、与全书机制的汇合

| 步骤 | 涉及章节 |
|------|----------|
| 权限 / setuid / CAP | 本章 §2 · Ch 10 |
| 新 mm、VMA、mmap 布局 | Ch 9 · 本章 §3 |
| 读 ELF 文件 | Ch 12 VFS · Ch 15 页缓存 · Ch 16 |
| 映射文件页 | Ch 9 缺页 · demand paging |
| 调度运行 | Ch 7 |

---

### 四、ULK 全书 20 章索引

| 章 | 主题 | 笔记 |
|----|------|------|
| 1 | 引言 | [chapter-01-introduction/](../chapter-01-introduction/) |
| 2 | 内存寻址 | [chapter-02-memory-addressing/](../chapter-02-memory-addressing/) 🔴 |
| 3 | 进程 | [chapter-03-processes/](../chapter-03-processes/) 🔴 |
| 4 | 中断与异常 | [chapter-04-interrupts-and-exceptions/](../chapter-04-interrupts-and-exceptions/) 🔴 |
| 5 | 内核同步 | [chapter-05-kernel-synchronization/](../chapter-05-kernel-synchronization/) 🔴 |
| 6 | 计时 | [chapter-06-timing/](../chapter-06-timing/) |
| 7 | 进程调度 | [chapter-07-process-scheduling/](../chapter-07-process-scheduling/) 🔴 |
| 8 | 内存管理 | [chapter-08-memory-management/](../chapter-08-memory-management/) 🔴 |
| 9 | 进程地址空间 | [chapter-09-process-address-space/](../chapter-09-process-address-space/) 🔴 |
| 10 | 系统调用 | [chapter-10-system-calls/](../chapter-10-system-calls/) 🔴 |
| 11 | 信号 | [chapter-11-signals/](../chapter-11-signals/) |
| 12 | VFS | [chapter-12-VFS/](../chapter-12-VFS/) |
| 13 | I/O 与驱动 | [chapter-13-io-architecture/](../chapter-13-io-architecture/) |
| 14 | 块设备 | [chapter-14-block-devices/](../chapter-14-block-devices/) |
| 15 | 页缓存 | [chapter-15-page-cache/](../chapter-15-page-cache/) |
| 16 | 文件访问 | [chapter-16-file-access/](../chapter-16-file-access/) |
| 17 | 页框回收 | [chapter-17-page-reclaim/](../chapter-17-page-reclaim/) |
| 18 | Ext2/Ext3 | [chapter-18-ext2-ext3/](../chapter-18-ext2-ext3/) |
| 19 | 进程通信 | [chapter-19-ipc/](../chapter-19-ipc/) |
| 20 | 程序执行 | 本章 |

**附录（待扩展）：**

- [附录 A 系统启动](../appendix-A-system-startup.md) — 加电 → init  
- [附录 B 内核模块](../appendix-B-modules.md) — 动态加载  

**深潜配套：** [07 Gorman VM](../../../05-Linux-Virtual-Memory-Manager/) · [05 LKD](../../../03-Linux-Kernel-Development/) · [08 TLPI](../../../06-The-Linux-Programming-Interface/) · [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/)

---

← [5. 可执行格式](./section-5-可执行格式与执行域.md) · [OUTLINE.md](../OUTLINE.md)
