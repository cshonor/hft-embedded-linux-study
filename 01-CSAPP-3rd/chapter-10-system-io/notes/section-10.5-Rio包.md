## 10.5 Rio 健壮读写（10.5.1–10.5.2）

### 为何需要 Rio

裸 `read`/`write` **不保证** 读满/写满 `n` 字节 — 应用层需循环；Rio 封装常见模式。

### 10.5.1 无缓冲函数

```c
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
```

- **`rio_readn`** — 直到读满 `n` 字节、EOF 或错误
- **`rio_writen`** — 直到写满 `n` 字节
- 处理 **EINTR** 自动重启

### 10.5.2 带缓冲输入

```c
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
```

- 内部 **static buffer** — 减少 `read` 系统调用次数
- **`rio_readlineb`** — 按行读（文本协议）；**二进制协议** 多用定长 `rio_readnb`

**HFT：**

| 场景 | 建议 |
|------|------|
| 定长二进制帧 | `rio_readn` / 自写 `read_full` |
| 文本 admin 命令 | `rio_readlineb` |
| 超高性能收包 | 不用 Rio — **环形缓冲 + 零拷贝**（DPDK/onload） |

- Rio 适合 **正确性模板**；生产引擎常自研 **ByteStream parser**，但语义同「读满 n」

---

← [本章导读](../README.md)
