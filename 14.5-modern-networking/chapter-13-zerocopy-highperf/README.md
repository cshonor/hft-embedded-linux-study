# Chapter 13: 零拷贝与高性能网络

> 来源：kernel-docs（scaling + msg_zerocopy）+ LWN（MSG_ZEROCOPY + TCP zero-copy recv + SO_REUSEPORT）
> 对标：Rosen（无零拷贝，3.x 无 MSG_ZEROCOPY）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [scaling](notes/01-scaling.md) | kernel-docs：RSS/RPS/RFS/XPS 扩展，多核扩展 |
| 2 | [msg-zerocopy](notes/02-msg-zerocopy.md) | kernel-docs：MSG_ZEROCOPY 发送零拷贝、completion 通知 |
| 3 | [msg-zerocopy-lwn](notes/03-msg-zerocopy-lwn.md) | LWN：MSG_ZEROCOPY 实现、DMA 直接到 NIC |
| 4 | [tcp-zero-copy-recv](notes/04-tcp-zero-copy-recv.md) | LWN：TCP zero-copy receive、mmap TCP 缓冲区 |
| 5 | [so-reuseport](notes/05-so-reuseport.md) | LWN：SO_REUSEPORT 多进程/多线程负载均衡 |

## HFT 关联

- **MSG_ZEROCOPY 发送**：避免内核拷贝数据到 sk_buff，DMA 直接从用户态内存发送；节省 ~100ns/包
- **TCP zero-copy recv**：`mmap` TCP 接收缓冲区到用户态，避免 recvmsg 的数据拷贝
- **SO_REUSEPORT**：多个进程 bind 同一端口，内核做 per-flow 负载均衡，避免单进程瓶颈
- **RSS/RFS**：RSS 硬件多队列 + RFS 软件路由，确保收包在正确 CPU 处理
- **HFT 完整方案**：AF_XDP 收包 + MSG_ZEROCOPY 发包 + SO_REUSEPORT 多进程

## 交叉引用

- `14.5-modern-networking/chapter-03-tx-path-skbbuff/`：发包路径与零拷贝
- `14.5-modern-networking/chapter-06-af-xdp/`：AF_XDP 零拷贝收包
- `12-network-sockets/`：SO_REUSEPORT 基础
