## 5. 异步 I/O (Asynchronous I/O)

> Linux 2.6 **内核原生** AIO — 与 POSIX AIO 呼应

---

### 一、基本模型

| 概念 | 说明 |
|------|------|
| **非阻塞提交** | 进程提交 I/O → **立即继续** 执行 |
| **完成通知** | 传输结束后由内核 **标记完成** — 用户再 `io_getevents` 等 |

---

### 二、核心对象与 syscall

| 组件 | 作用 |
|------|------|
| **`kiocb`** | 跟踪 **单次** 挂起异步 I/O 的状态 |
| **`io_setup()`** | 创建 **AIO context** |
| **`io_submit()`** | 批量提交多个 I/O 请求 |
| **`io_getevents()`** | 收割已完成事件（ULK/TLPI 用户态） |

内核与用户态通过 **AIO 环形缓冲区** 等机制共享完成状态。

---

### 三、与 `O_DIRECT` 的配合

若文件 **`O_DIRECT`** 打开：

- 内核可 **直接** 触发磁盘传输  
- 提交 syscall **快速返回**  
- 完成状态写入共享环  

绕过页缓存 — 与 [section-2](./section-2-文件访问模式.md) 直接 I/O 一致。

---

### 四、局限与 modern 对照

ULK 2.6 AIO：

- 主要面向 **O_DIRECT**、块对齐场景  
- 网络 AIO 等支持有限  

**io_uring**（5.1+）提供 **统一、高性能** 异步接口 — HFT 新设计可优先评估 io_uring 而非 legacy AIO。

→ syscall 层：[Ch 10](../chapter-10-system-calls/)

---

← [4. mmap](./section-4-内存映射.md) · 下一节 [6. 全路径串联](./section-6-全路径串联与索引.md)
