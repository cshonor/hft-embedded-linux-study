## 11.4 套接字接口（11.4.1–11.4.9）

### 11.4.1 套接字地址结构

```c
struct sockaddr_in {
    sa_family_t    sin_family;  // AF_INET
    in_port_t      sin_port;    // htons
    struct in_addr sin_addr;    // htonl
    char           sin_zero[8];
};
```

- 通用指针 `struct sockaddr *` — API 多态；IPv6 用 `sockaddr_in6`

### 11.4.2 socket

```c
int socket(int domain, int type, int protocol);
// AF_INET, SOCK_STREAM, 0  → TCP
// AF_INET, SOCK_DGRAM, 0   → UDP
```

### 11.4.3 connect（客户端）

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

- 发起 **TCP 三次握手**；成功后可 `read`/`write`

### 11.4.4 bind（服务器）

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

- 绑定本地 **IP + 端口**；`INADDR_ANY` 监听所有网卡

### 11.4.5 listen

```c
int listen(int sockfd, int backlog);
```

- **被动** 套接字；`backlog` 已完成握手等待 `accept` 的队列长度

### 11.4.6 accept

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

- 返回 **新 fd** — 与客户端通信；监听 fd 继续 `accept`

### 11.4.7 主机和服务转换 — getaddrinfo

```c
int getaddrinfo(const char *host, const char *service,
                const struct addrinfo *hints, struct addrinfo **result);
void freeaddrinfo(struct addrinfo *result);
```

- **协议无关**、线程安全；替代 `gethostbyname`
- `hints.ai_socktype = SOCK_STREAM` 等过滤

```c
// 解析 "example.com:8080"
getaddrinfo(host, port, &hints, &res);
for (p = res; p; p = p->ai_next)
    if ((fd = socket(...)) >= 0 && connect(fd, p->ai_addr, ...) == 0)
        break;
```

### 11.4.8 辅助函数

- `getnameinfo` — 反向解析
- `inet_pton` / `inet_ntop` — 文本 ↔ 二进制 IP

### 11.4.9 Echo 客户端与服务器

**服务器骨架：**

```c
listenfd = socket(...);
bind(listenfd, ...);
listen(listenfd, backlog);
while (1) {
    connfd = accept(listenfd, ...);
    // read loop echo write — 用 rio_readlineb / rio_writen
    close(connfd);
}
```

**客户端：** `socket` → `connect` → `rio_writen`/`rio_readlineb` → `close`

**HFT 延伸（本章未展开，生产必备）：**

| 技术 | 用途 |
|------|------|
| `O_NONBLOCK` + **`epoll`** | 单线程多连接 |
| **`TCP_NODELAY`** | 禁 Nagle，降小包延迟 |
| **`SO_REUSEPORT`** | 多进程收包 |
| **`SO_BUSY_POLL`** | 低延迟 busy poll（慎用 CPU） |
| **UDP multicast** | 交易所组播行情 |

→ [12-UNP](../../../03.5-unix-network-api/) · [16-Systems-Performance Ch10](../../../15-systems-performance/chapter-10-network/) · [18-HFT ch06](../../../17-hft-engineering/)

### 常见陷阱
1. **socket 返回 fd 但尚未连接** — 还需要 connect（客户端）或 bind+listen+accept（服务器）
2. **accept 返回新 fd** — 监听 fd 继续接受新连接，不能用它收发数据
3. **getaddrinfo 是协议无关的** — 同一代码兼容 IPv4/IPv6，替代废弃的 gethostbyname

### 自测题

<details>
<summary>Q1: socket() 返回什么？此时连接建立了吗？</summary>

返回一个 fd（文件描述符），但此时尚未连接。客户端需 connect() 发起连接，服务器需 bind()+listen()+accept() 等待连接。

</details>

<details>
<summary>Q2: accept() 返回的 fd 和监听 fd 有什么区别？</summary>

accept() 返回一个新的 fd（connected fd），用于与客户端通信。监听 fd 继续接受新连接，不能用于收发数据。一个监听 fd 可对应多个 connected fd。

</details>

<details>
<summary>Q3: 为什么推荐 getaddrinfo 而不是 gethostbyname？</summary>

getaddrinfo 协议无关（自动处理 IPv4/IPv6）、线程安全（gethostbyname 返回静态缓冲区，非线程安全）、支持服务名解析。gethostbyname 已废弃。

</details>

<details>
<summary>Q4: HFT 常用的 socket 选项有哪些？各解决什么问题？</summary>

TCP_NODELAY：禁 Nagle 算法，降小包延迟。SO_REUSEPORT：多进程/线程同时收包。SO_BUSY_POLL：内核 busy poll，减少中断延迟。O_NONBLOCK+epoll：单线程管理多连接。

</details>
---

← [本章导读](../README.md)
