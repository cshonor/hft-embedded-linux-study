## ① Unix 的历史 · History of Unix

| 事实 | 说明 |
|------|------|
| **起源** | **1969** · 贝尔实验室 · **Dennis Ritchie**、**Ken Thompson** |
| **成功因素** | 见下表 |

| Unix 优势 | 含义 |
|-----------|------|
| **设计简洁** | 仅 **~几百个系统调用** — 接口少而稳 |
| **一切皆文件** | 设备、socket、管道… 统一 **open/read/write** |
| **C 语言实现** | **可移植** — 换硬件主要重编译内核 |
| **极快进程创建** | 独特 **`fork()`** — 复制地址空间语义 |
| **稳健 IPC** | 管道、信号等 **简单原语** |

#### 时间线（考据锚点）

```
1969  Thompson 在 PDP-7 上写 Unix（汇编）
1973  Ritchie 用 C 重写 —— 跨硬件移植的门被打开
1977  Berkeley Software Distribution (BSD) 分支起步
1983  System V（AT&T 商业版）vs BSD —— 「Unix 战争」
1988  POSIX 诞生 —— 停战方案：只标准化 API，不管实现
1991  Linux 以 GPL 重做类 Unix 内核 —— 吸收两者遗产
```

| 分叉 | 留给 Linux 的遗产 |
|------|------------------|
| System V | `SysV IPC`（shm/sem/msg）、init 体系 |
| BSD | sockets 网络栈、`mmap`、virtual memory 布局 |
| POSIX | 今天的可移植性契约——Linux syscall 表基本按 POSIX 覆盖设计 |

#### 五条优势的内核机制对应

| 哲学 | 内核里的落点 | 现代演化 |
|------|--------------|----------|
| 一切皆文件 | VFS 的 `file_operations` 抽象（Ch 13） | io_uring 用**队列**而非 fd 流——抽象开始让位于延迟 |
| C 实现 | 内核至今 C 为主（+少量 Rust，6.1+） | Rust for drivers 正在写入主线 |
| fork 语义 | 写时拷贝 COW（[3.4](../../chapter-03-process-management/notes/section-3.4-进程创建与写时拷贝.md)） | `posix_spawn`/`vfork` 是后辈对 fork 成本的补丁 |
| 少 syscall | `read/write/mmap` 覆盖 90% 场景 | io_uring 一个入口复用百种操作 |

**HFT 对照：** 网关仍活在 **「少 syscall、少拷贝、快 fork/线程」** 的 Unix 遗产里 — 热路径 **`read`/`send`/`mmap`** 皆是「一切皆文件」后代；而绕开它的两条路（DPDK 用户态轮询、io_uring 直投队列）本质都是**对 Unix 抽象税的 revolt**——先懂税再谈逃税。

→ [03-linux-userspace-api](../../../03-linux-userspace-api/) · [02-CSAPP Ch8](../../../02-computer-systems/chapter-08-exceptional-control-flow/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Unix 只有几百个系统调用，这为什么对 HFT 有利？

<details><summary>答案</summary>

系统调用少意味着 syscall 入口审计面小、热路径可预测。HFT 网关热路径只用 read/write/mmap/send 等十几个 syscall，减少调度器与 VFS 层分支。少而稳的接口 = 内核版本升级时 ABI 兼容性好。

</details>

**Q2.** Unix「一切皆文件」设计哲学对现代网络编程有什么局限？

<details><summary>答案</summary>

一切皆文件让 socket/fd 统一接口，但网络包仍需从内核拷贝到用户态（除非用 zero-copy）。HFT 用 AF_XDP/DPDK 绕过 VFS 直接从网卡取包，就是因为「文件抽象」对纳秒级延迟仍有开销。

</details>

**Q3.** fork() 的「独特」体现在哪？它的性能问题后来是怎么被逐步修补的？

<details><summary>答案</summary>

独特在**复制整个地址空间的语义**（子进程获得父进程的完整副本）——比 spawn（传可执行路径重新加载）语义强大得多：可以在 fork 后、exec 前对副本做任意操作（重定向、改环境）。修补史：① **vfork**（BSD）——不复制，父挂起等 exec，过渡方案；② **COW**（现代 fork）——只复制页表不复制页，写时才拷——fork 成本从 O(内存) 降到 O(页表)；③ **posix_spawn**——COW 时代仍有场景省 fork（单页进程池）；④ **线程**（clone 共享地址空间）——彻底绕开复制语义。链条读法：语义的优雅先用，性能的账慢慢还。

</details>

</details>
---
