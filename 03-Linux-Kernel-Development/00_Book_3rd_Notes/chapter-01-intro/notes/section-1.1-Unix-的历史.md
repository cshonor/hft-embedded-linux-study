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

→ [06-The-Linux-Programming-Interface](../../../../06-The-Linux-Programming-Interface/) · [01-CSAPP Ch8](../../../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/)

---
