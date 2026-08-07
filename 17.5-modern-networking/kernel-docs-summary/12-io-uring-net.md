# 12 — io_uring 网络

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/io_uring/`（部分网络操作分散在各子系统文档）

## 文档概述

io_uring 网络操作相关文档，涵盖异步 accept/recv/send/connect 等。

## 核心内容

### 网络相关 opcode

| Opcode | 内核版本 | 对应系统调用 |
|--------|---------|------------|
| IORING_OP_ACCEPT | 5.5 | accept4() |
| IORING_OP_RECV | 5.6 | recv()/recvfrom() |
| IORING_OP_RECVMSG | 5.6 | recvmsg() |
| IORING_OP_SEND | 5.6 | send()/sendto() |
| IORING_OP_SENDMSG | 5.6 | sendmsg() |
| IORING_OP_CONNECT | 5.6 | connect() |
| IORING_OP_SEND_ZC | 6.0 | sendmsg 零拷贝 |

### SQPOLL 模式

```c
struct io_uring_params params = {
    .flags = IORING_SETUP_SQPOLL,
    .sq_thread_idle = 10000,  // 空闲 10 秒后退出
};
io_uring_setup(ENTRIES, &params);
```
- 内核创建 sqpoll 线程持续轮询 SQ ring
- 用户态不需要 `io_uring_enter()`，零系统调用
- 适合低延迟场景（但消耗一个 CPU）

### registered buffers

```c
struct iovec iovecs[N] = { ... };
io_uring_register_buffers(ring, iovecs, N);
```
- 预注册用户态 buffer，内核 pin 住 page
- 后续 recv/send 直接使用注册 buffer
- 避免 `get_user_pages()` 开销

### multishot accept（5.19+）

```c
io_uring_prep_multishot_accept(sqe, sockfd, addr, addrlen, flags);
```
- 一次提交，多次完成
- 每次新连接自动生成 CQE
- 减少 accept 的 SQE 提交开销

## HFT 要点

- SQPOLL + registered buffers = 最低延迟网络 IO
- SEND_ZC = 异步零拷贝发送（优于 MSG_ZEROCOPY 的同步通知）
- multishot accept 适合多连接行情源
