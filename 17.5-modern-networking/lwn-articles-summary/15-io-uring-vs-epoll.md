# 15 — io_uring vs epoll：性能对比

> **对应 Rosen:** 无
> **内核版本:** epoll 2.6+；io_uring 5.1+

## 事件模型 vs 完成模型

**epoll（事件模型）：**
```
epoll_wait() → 通知 socket 可读 → recvmsg() → 数据就绪
     ↑ 系统调用 1              ↑ 系统调用 2
```
- 两次系统调用：epoll_wait + recvmsg
- epoll 只通知"可读"，实际读取仍需调用 recv

**io_uring（完成模型）：**
```
提交 recv SQE → 内核异步完成 → CQ 通知数据就绪
  ↑ 0 次系统调用（SQPOLL）或 1 次
```
- 一次提交包含完整操作（recv），内核完成后通知
- SQPOLL 模式下零系统调用

## 性能对比（单连接）

| 指标 | epoll + recvmsg | io_uring (SQPOLL) |
|------|----------------|-------------------|
| 系统调用数 | 2（epoll_wait + recv） | 0（SQPOLL 自旋） |
| 延迟 | ~1-2 μs | ~0.5-1 μs |
| CPU 占用 | 事件驱动（可休眠） | 100%（SQPOLL 线程） |

## 性能对比（多连接）

| 连接数 | epoll 优势 | io_uring 优势 |
|--------|-----------|-------------|
| < 100 | 简单够用 | 批量提交减少系统调用 |
| 100-10000 | 仍可接受 | 批量优势明显 |
| > 10000 | 性能下降 | 优势扩大 |

## HFT 适用性

| 场景 | 推荐 | 原因 |
|------|------|------|
| 行情接收（少量连接） | io_uring SQPOLL | 零系统调用，最低延迟 |
| 交易发送（少量连接） | io_uring SEND_ZC | 零拷贝 + 异步 |
| 管理通道（多连接） | epoll | 简单，不需要 100% CPU |
| 兼容性要求 | epoll | io_uring 需 5.5+ 内核 |

## 代码复杂度对比

epoll 代码更简单，io_uring 需要：
- 初始化 io_uring 实例 + 注册 buffer
- 管理 SQE/CQE 生命周期
- 处理 buffer recycling

> HFT 建议：行情/交易路径用 io_uring（SQPOLL + 零拷贝），管理通道用 epoll。
