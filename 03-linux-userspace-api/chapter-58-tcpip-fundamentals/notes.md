# TLPI 第 58 章 — Sockets: Fundamentals of TCP/IP Networks

> 对应目录：`chapter-58-tcpip-fundamentals/`  
> 书名原文：**Sockets: Fundamentals of TCP/IP Networks**  
> ⚠️ **理论章：INET 编程前必会。** 端口/IP 一律 **网络字节序（大端）** → `htons`/`htonl`。用 **`inet_pton`/`inet_ntop`**，勿用 `inet_ntoa`。UDP 会丢；**UNIX DGRAM 才可靠**。TCP=字节流无边界。

**优先级**：🟡（Ch59 实战地基）  
**前置**：[Ch57 UNIX 域](../chapter-57-sockets-unix-domain/notes.md)  
**后置**：[Ch59 Internet Domains](../chapter-59-internet-domains/notes.md)

---

## 章节目标

IPv4/端口；`sockaddr_in`；字节序；pton/ntop；TCP vs UDP；`sockaddr_storage`；`INADDR_ANY`。

---

## 58.1 INET vs UNIX

| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 定位 | 路径/抽象名 | **IP + 端口** |
| 范围 | 仅本机 | 跨机 / 回环 |

`SOCK_STREAM`→TCP；`SOCK_DGRAM`→UDP。

---

## 58.2–58.3 IPv4 · 端口

- 32 位；点分十进制；`127.0.0.1` 回环（不经物理网卡）  
- **`INADDR_ANY` (0.0.0.0)**：服务端 bind 全网卡；**客户端 connect 不用**  
- 私有：`10/8`、`172.16/12`、`192.168/16`  

端口 0–65535：知名 0–1023（常需 root）；注册 1024–49151；临时 49152–65535（客户端常自动分配，**一般不手 bind**）。

---

## 58.4 `sockaddr_in`

```c
struct sockaddr_in {
    sa_family_t    sin_family;  /* AF_INET */
    in_port_t      sin_port;    /* 网络序 */
    struct in_addr sin_addr;    /* s_addr 网络序 */
    unsigned char  sin_zero[8]; /* 置 0 */
};
```

传给 bind/connect 时转为 `struct sockaddr *`。

---

## 58.5 网络字节序（高频）

网络固定 **大端**；主机常见小端。  
`htons`/`htonl` / `ntohs`/`ntohl` — **永远调用**，勿猜本机端序。

---

## 58.6 地址转换

| 推荐 | `inet_pton` / `inet_ntop`（IPv4+IPv6） |
|------|----------------------------------------|
| 废弃倾向 | `inet_addr`/`aton`/`ntoa`（仅 v4；`ntoa` 静态缓冲、非线程安全） |

`pton`：返回 1 成功，0 非法串，-1 错误。

Demo：[`code/`](./code/)

---

## 58.7 TCP vs UDP

| | TCP | UDP |
|--|-----|-----|
| 连接 | 有（握手/挥手） | 无 |
| 可靠 | ACK/重传/去重 | 可丢/乱序/重复 |
| 模型 | 字节流 | 报文边界 |
| 控流 | 有 | 无 |
| 场景 | HTTP/SSH/文件 | DNS/直播/低延迟容忍丢包 |

再记：UNIX DGRAM **可靠**；INET UDP **不可靠**。

---

## 58.8–58.9 `sockaddr_storage` · `INADDR_ANY`

`sockaddr_storage`：够大装 v4/v6，写双栈通用代码。  
服务端：`sin_addr.s_addr = htonl(INADDR_ANY)`。

---

## 初始化模板

```c
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
/* or: addr.sin_addr.s_addr = htonl(INADDR_ANY); */
```

---

## 陷阱

1. 忘 htons/htonl  
2. 用 inet_ntoa  
3. 当 UDP 可靠  
4. TCP 一次 send≠一次 recv  
5. sin_zero 未清零  
6. 客户端 connect `INADDR_ANY`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 端点 = IP + 端口 |
| 2 | 网络大端；htons/htonl |
| 3 | pton/ntop；勿 ntoa |
| 4 | INADDR_ANY 仅服务端 bind |
| 5 | TCP 流 / UDP 报；UDP 可丢 |
| 6 | UNIX DGRAM ≠ UDP |

---

## 参考

- Kerrisk · TLPI Ch58  
- `man 3 htons` · `inet_pton` · `man 7 ip` · `tcp` · `udp`
