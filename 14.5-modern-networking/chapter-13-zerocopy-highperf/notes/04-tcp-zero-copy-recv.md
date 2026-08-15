# 17 — TCP zero-copy 接收

> **对应 Rosen:** Ch11（recvmsg 只有拷贝模式）
> **内核版本:** TCP zero-copy receive 5.0+（tcp_recvmsg MSG_ZEROCOPY 实为 mmap 方案）

## 传统接收路径

```
NIC DMA → 内核 page → copy_to_user() → 用户态 buffer
```
- recvmsg() 将内核 sk_buff 数据拷贝到用户态
- 高吞吐场景拷贝开销大

## TCP zero-copy 接收方案

Linux 提供两种 TCP 零拷贝接收方式：

### 方案 1：TCP mmap（`TCP_ZEROCOPY_RECEIVE`，4.18+）

```c
struct tcp_zerocopy_receive zc = {
    .address = (uintptr_t)mapped_buffer,
    .length = buffer_size,
};
setsockopt(sockfd, IPPROTO_TCP, TCP_ZEROCOPY_RECEIVE,
           &zc, sizeof(zc));
```
- 内核将 TCP 接收队列中的 page 映射到用户态地址空间
- 用户态直接读取映射内存，无需拷贝
- 读取完后通知内核（offset 更新）

### 方案 2：io_uring registered buffers（5.7+）

```c
// 注册用户态 buffer 到 io_uring
struct iovec iov = { .iov_base = buf, .iov_len = buf_size };
io_uring_register_buffers(ring, &iov, 1);

// 提交 recv 直接写入注册 buffer
io_uring_prep_recv(sqe, sockfd, buf, buf_size, 0);
```
- 内核直接将数据写入预注册的用户态 buffer
- 无需拷贝（DMA → 用户态 page）

## 性能对比

| 方案 | 拷贝次数 | 延迟 | 复杂度 |
|------|---------|------|--------|
| 传统 recvmsg | 1（内核→用户） | 基准 | 低 |
| TCP_ZEROCOPY_RECEIVE | 0（mmap） | 略低 | 中 |
| io_uring + registered buf | 0（DMA 直达） | 最低 | 高 |

## HFT 关联

| 场景 | 推荐方案 |
|------|---------|
| 行情接收（小包） | 传统 recvmsg（小包拷贝开销可忽略） |
| 行情转发（大包） | TCP_ZEROCOPY_RECEIVE 或 io_uring |
| AF_XDP | 最优（根本不经过 TCP 协议栈） |
