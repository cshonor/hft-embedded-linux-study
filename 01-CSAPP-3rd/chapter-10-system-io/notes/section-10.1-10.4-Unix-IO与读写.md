## 10.1–10.4 Unix I/O 与读写

### 10.1 Unix I/O

- **一切皆文件** — 普通文件、目录、设备、**socket**、管道
- **文件描述符 (fd)** — 小整数，进程 **fd 表** 中的索引

| fd | 默认 |
|----|------|
| 0 | stdin |
| 1 | stdout |
| 2 | stderr |

### 10.2 文件

- **字节序列** — 从 0 开始隐式编号
- **文件类型** — 普通、目录、符号链接等（`stat` 的 `S_IF*`）

### 10.3 打开和关闭

```c
int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
```

常用 `flags`：

| 标志 | 含义 |
|------|------|
| `O_RDONLY` / `O_WRONLY` / `O_RDWR` | 访问模式 |
| `O_CREAT` | 不存在则创建 |
| `O_TRUNC` | 截断为 0 |
| `O_APPEND` | 写追加到末尾 |
| `O_NONBLOCK` | 非阻塞（网络常用，Ch11） |

- `close` 释放 fd；**fd 耗尽** `EMFILE` — 网关要调 `ulimit -n`

### 10.4 读和写

```c
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
```

- 返回 **实际字节数**；0 表示 EOF；**-1** 错误（查 `errno`）
- **短计数 (short count)** — `read` 请求 1000 字节可能只返回 42  
  原因：管道/socket 缓冲、信号中断、接近 EOF 等

**HFT：**

- 二进制协议 **必须循环读满** 或状态机 — 不能假设一次 `read` = 一帧
- **`EINTR`** — 被信号打断，重试（→ [Ch 8](../../chapter-08-exceptional-control-flow/notes/section-8.3-系统调用错误处理.md)）
- 热路径 eventual **非阻塞** + 事件循环（Ch11）

---

← [本章导读](../README.md)
