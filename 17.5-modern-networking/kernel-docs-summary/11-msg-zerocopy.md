# 11 — Documentation/networking/msg_zerocopy.rst

> **对应 Rosen:** Ch11（sendmsg 拷贝模式）
> **内核源码路径:** `Documentation/networking/msg_zerocopy.rst`

## 文档概述

MSG_ZEROCOPY 官方文档，描述零拷贝发送机制和通知接口。

## 核心内容

### 启用零拷贝

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt));
// 或在 sendmsg flags 中传入 MSG_ZEROCOPY
sendmsg(sockfd, &msg, MSG_ZEROCOPY);
```

### 通知机制

零拷贝发送后，buffer 不能立即释放。内核通过 errqueue 通知完成：

```c
// 接收完成通知
struct msghdr msg = {};
char control[CMSG_SPACE(sizeof(struct sock_extended_err))];
msg.msg_control = control;
msg.msg_controllen = sizeof(control);
recvmsg(sockfd, &msg, MSG_ERRQUEUE);

// 解析完成范围
struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
struct sock_extended_err *serr = (void *)CMSG_DATA(cm);
// serr->ee_data = 完成的范围起始序号
// serr->ee_info = 完成的范围结束序号
```

### 适用条件

| 条件 | 要求 |
|------|------|
| 网卡 | 支持 checksum offload + scatter-gather |
| 协议 | TCP / UDP（4.14+ TCP，5.x UDP） |
| 数据量 | > 4KB 才有收益（小包通知开销 > 拷贝开销） |

### 性能数据

| 数据量 | 拷贝模式 | 零拷贝 | 收益 |
|--------|---------|--------|------|
| 1 KB | 1.0 μs | 1.5 μs | -50%（更慢） |
| 4 KB | 1.5 μs | 1.2 μs | +20% |
| 64 KB | 5.0 μs | 1.5 μs | +230% |
| 1 MB | 80 μs | 3.0 μs | +2500% |

## HFT 要点

- 交易报文（< 1KB）：不用零拷贝（通知开销 > 拷贝开销）
- 行情转发（大包）：零拷贝收益大
- io_uring SEND_ZC（6.0+）是更好的零拷贝发送方案（异步 + 批量通知）
