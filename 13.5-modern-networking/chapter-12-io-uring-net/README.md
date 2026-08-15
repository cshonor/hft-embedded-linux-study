# Chapter 12: io_uring 网络收发

> 来源：kernel-docs（io_uring net）+ LWN（io_uring net + vs epoll）
> 对标：Rosen（无 io_uring，3.x 仅 epoll）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [io-uring-net](notes/01-io-uring-net.md) | kernel-docs：io_uring 网络操作、recvmsg/sendmsg、multishot |
| 2 | [io-uring-net-lwn](notes/02-io-uring-net-lwn.md) | LWN：io_uring 网络设计、SQE/CQE、注册缓冲区 |
| 3 | [io-uring-vs-epoll](notes/03-io-uring-vs-epoll.md) | LWN：io_uring vs epoll 性能对比、批量提交优势 |

## HFT 关联

- **io_uring 批量提交**：多个 send/recv 打包成一次系统调用，减少 syscall 开销（100ns → 20ns/操作）
- **注册缓冲区**：io_uring registered buffers 避免每次 recvmsg 的 mmap 开销
- **multishot recv**：一次 SQE 持续接收多个包，适合行情订阅场景
- **io_uring vs epoll**：
  - epoll：事件驱动，每个就绪 fd 需单独 recvmsg（2 次 syscall）
  - io_uring：批量提交，一次 io_uring_enter 处理 N 个操作
- **HFT 选择**：io_uring 适合批量收发场景，AF_XDP 仍是最快路径

## 交叉引用

- `04.5-network-sockets/`：epoll/socket 基础
- `13.5-modern-networking/chapter-13-zerocopy-highperf/`：io_uring + 零拷贝组合
