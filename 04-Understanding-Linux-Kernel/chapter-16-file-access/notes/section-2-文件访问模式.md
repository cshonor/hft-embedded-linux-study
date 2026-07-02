## 2. 文件访问的多种模式

> 同一文件 — **五种** 不同的内核/缓存语义

---

### 一、规范模式 (Canonical) — 默认

| 操作 | 行为 |
|------|------|
| **`read()`** | 经 **页缓存**；阻塞直到拷贝到 **用户态** |
| **`write()`** | 拷贝到页缓存 → 标 **脏** → **延迟写** 返回 |

→ 页缓存：[Ch 15](../chapter-15-page-cache/) · syscall：[Ch 10](../chapter-10-system-calls/)

---

### 二、同步模式 — `O_SYNC`

打开时设 **`O_SYNC`**（及相关的 `O_DSYNC` 等）：

- 仍更新页缓存  
- **`write` 阻塞** 直到数据 **物理落盘**（或 FS 等价持久化点）  

比默认 write  **慢**，语义 **强** — 类似应用层 `fsync` 每写一次。

---

### 三、内存映射 — `mmap()`

将文件映射进 **进程虚拟地址空间**：

- 应用像 **读写内存数组** 一样访问文件  
- **绕过** `read()` / `write()` syscall 循环  

底层仍常经 **页缓存**（共享映射）— 见 [section-4](./section-4-内存映射.md)。

→ VMA / mmap：[Ch 9](../chapter-09-process-address-space/notes/section-3-内存区VMA.md)

---

### 四、直接 I/O — `O_DIRECT`

| 特点 | 说明 |
|------|------|
| **绕过页缓存** | 用户缓冲区 ↔ 磁盘 **直接** 传输 |
| **典型用户** | 自带 buffer pool 的 **数据库** |
| **约束** | 缓冲区常须 **对齐**（扇区/页）；内核 **pin** 用户页做 DMA |

> **深潜可选：** `O_DIRECT` → `blockdev_direct_IO()` — 锁定用户页、构造 bio。

HFT：自管内存池 + 日志盘时可考虑；需处理对齐与 **fsync** 语义。

---

### 五、异步 I/O — AIO

发起 I/O 后 **立即返回**，不阻塞等待完成 — 见 [section-5](./section-5-异步IO.md)。

> **Modern 对照：** Linux **io_uring** 远超 2.6 POSIX AIO；ULK 讲 **kiocb 模型** 抓概念。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 读写与预读](./section-3-读写与预读.md)
