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

**HFT 对照：** 网关仍活在 **「少 syscall、少拷贝、快 fork/线程」** 的 Unix 遗产里 — 热路径 **`read`/`send`/`mmap`** 皆是「一切皆文件」后代。

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

</details>
---
