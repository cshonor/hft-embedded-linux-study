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

### 自测题

<details>
<summary>1. Rio 包的 `rio_readn` 和 `rio_readnb` 有什么区别？</summary>

`rio_readn(fd, buf, n)`：**无缓冲**，直接调用 `read` 循环直到读满 n 字节。适合定长二进制帧。
`rio_readnb(rp, buf, n)`：**有缓冲**，从内部 buffer 读，不够再调 `read` 补充。适合文本协议（配合 `rio_readlineb` 按行读）。

**关键区别**：`rio_readn` 不能和 `rio_readlineb` 混用（会丢缓冲区数据），因为前者不经过 buffer。HFT 定长帧用 `rio_readn`，文本 admin 用 `rio_readlineb`+`rio_readnb`。

</details>

<details>
<summary>2. Rio 带缓冲读取为什么能减少系统调用次数？</summary>

Rio 内部维护一个 `char rio_buf[8192]` 缓冲区。`rio_readlineb` 先从 buffer 读，buffer 空了才调一次 `read` 填满 8192 字节。这样 1000 次按行读可能只触发几十次 `read` 系统调用。每次系统调用 ~1μs，省下毫秒级延迟。HFT 注意：缓冲不适合超低延迟场景（DPDK 零拷贝），但是正确性和性能的好折中。

</details>


---

← [本章导读](../README.md)
