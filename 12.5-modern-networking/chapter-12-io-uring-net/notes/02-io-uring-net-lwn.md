# 14 — io_uring 网络收发接口

> **对应 Rosen:** 无
> **内核版本:** io_uring 5.1+；网络操作 5.5+；multishot 5.19+

## io_uring 是什么

io_uring 是 Linux 的异步 IO 框架：
- 两个环形队列：SQ（提交队列）+ CQ（完成队列）
- 用户态程序通过共享内存提交 IO 请求，无需系统调用
- 内核异步完成后写入 CQ，用户态轮询读取

## io_uring 网络操作

| 操作 | opcode | 内核版本 | 说明 |
|------|--------|---------|------|
| accept | IORING_OP_ACCEPT | 5.5 | 异步 accept() |
| recv/recvmsg | IORING_OP_RECV / RECVMSG | 5.6 | 异步接收 |
| send/sendmsg | IORING_OP_SEND / SENDMSG | 5.6 | 异步发送 |
| connect | IORING_OP_CONNECT | 5.6 | 异步 connect |
| multishot accept | IORING_OP_ACCEPT + multishot | 5.19 | 一次提交多次完成 |
| sendzc | IORING_OP_SEND_ZC | 6.0 | 零拷贝发送 |

## io_uring 网络收发流程

```
用户态:
  1. 准备 recv SQE → 写入 SQ ring
  2. io_uring_enter() 通知内核（或 SQPOLL 模式下内核自旋轮询）
  3. 轮询 CQ ring 等待完成事件

内核态:
  1. 从 SQ ring 读取 recv 请求
  2. 调用 socket 的 recvmsg 回调
  3. 数据就绪后写入 CQ ring
```

## SQPOLL 模式（内核轮询线程）

io_uring 可以创建一个内核线程持续轮询 SQ：
- 用户态不需要调用 `io_uring_enter()`，零系统调用提交
- 适合 HFT 低延迟场景（但消耗一个 CPU 核心）
- `IORING_SETUP_SQPOLL` flag 启用

## HFT 关联

| 维度 | io_uring 优势 |
|------|-------------|
| 系统调用开销 | 零（SQPOLL 模式）或 1 次/批（非 SQPOLL） |
| 批量操作 | 多个 recv/send 一次提交 |
| 异步等待 | 不阻塞，CQ 通知完成 |
| 零拷贝发送 | SEND_ZC（6.0+） |

## io_uring vs epoll

| 维度 | epoll | io_uring |
|------|-------|---------|
| 事件模型 | 事件通知（就绪后 read/write） | 完成模型（直接完成操作） |
| 系统调用 | epoll_wait + read/write | io_uring_enter（或 SQPOLL 零调用） |
| 数据拷贝 | read/write 仍需拷贝 | 可配合零拷贝（SEND_ZC） |
| 批量 | 不支持 | 一次提交多个操作 |
| 复杂度 | 低 | 中 |
