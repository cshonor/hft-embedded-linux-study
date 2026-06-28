## 2. 管道与 FIFO (Pipes and FIFOs)

> **生产者 / 消费者** — 单向字节流

---

### 一、管道 (Pipe)

| 项目 | 说明 |
|------|------|
| 创建 | **`pipe()`** — 一对 fd：**读端 / 写端** |
| 范围 | 通常 **有共同祖先** 的进程（fork 后共享） |
| 实现 | 内存中缓冲 — **`pipe_inode_info`** |

**Linux 2.6.11 改进：** **16 个 pipe buffer** 的 **环形数组** — 大块写入性能显著提升。

→ 特殊 FS pipefs：[Ch 12 section-5](../chapter-12-VFS/notes/section-5-挂载与路径查找.md) · syscall：[Ch 10](../chapter-10-system-calls/)

---

### 二、原子写（POSIX）

若 **多进程同时写** 同一管道：

- **小于 4096 字节**（一个 pipe buffer 大小）的 **`write`** 必须 **原子** — **不与** 其他进程数据 **交错**  
- 更大写可能分片 — 应用需自行组帧  

> **深潜可选：** `pipe_write` 对 `pipe->mutex` / 等待队列的加锁顺序。

---

### 三、FIFO（命名管道）

| vs 管道 | FIFO |
|---------|------|
| 无文件名 | **`mknod()` / `mkfifo()`** 在 FS 中创建 **节点** |
| 仅亲属进程 | **任意无关** 进程通过 **路径** open 通信 |

使用与普通文件相同的 **`open` / `read` / `write`** — [Ch 16](../chapter-16-file-access/)。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. SysV 基础](./section-3-System-V-IPC基础.md)
