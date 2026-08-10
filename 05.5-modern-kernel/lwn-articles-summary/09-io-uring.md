# io_uring — 新一代异步 I/O 接口

> **原文:** [io_uring](https://lwn.net/Articles/776703/) (LWN, 2019)
> **作者:** Jens Axboe
> **内核版本:** 5.1+ (引入), 6.x (功能持续扩展)
> **对标旧书:** ULK3 Ch16 (AIO)

---

## 核心观点

io_uring 是 Linux 5.1 引入的全新异步 I/O 接口，通过**共享内存环形缓冲区**实现用户态和内核之间的高效通信，几乎完全消除系统调用开销。

### 传统 AIO 的问题

| 问题 | 说明 |
|------|------|
| 需要系统调用 | 每次 I/O 提交和完成都需要系统调用 |
| 缓冲 I/O 不支持 | AIO 只支持 O_DIRECT，缓冲 I/O（页缓存命中）会阻塞 |
| API 复杂 | `io_submit()` / `io_getevents()` 接口笨重 |
| 不够通用 | 不支持 epoll、stat、open 等操作 |

### io_uring 的核心设计

**两个共享内存环形缓冲区：**

```
用户空间                          内核
┌──────────┐    ┌──────────┐
│ 提交队列  │───→│ 读取 SQE │    提交 I/O 请求
│ (SQ)     │    │ 执行 I/O │
└──────────┘    └──────────┘
                ┌──────────┐
│ 完成队列  │←───│ 写入 CQE │    返回 I/O 结果
│ (CQ)     │    └──────────┘
└──────────┘
```

| 队列 | 生产者 | 消费者 | 数据 |
|------|--------|--------|------|
| **SQ (Submission Queue)** | 用户态 | 内核 | SQE 索引数组 |
| **CQ (Completion Queue)** | 内核 | 用户态 | CQE 完成事件 |

### 关键数据结构

```c
// 提交队列条目 (SQE)
struct io_uring_sqe {
    __u8  opcode;     // IORING_OP_READV / WRITEV / NOP / ...
    __s32 fd;         // 文件描述符
    __u64 off;        // 偏移量
    void  *addr;      // 缓冲区
    __u32 len;        // 长度
    __u64 user_data;  // 关联回 SQE 的用户数据
    // ...
};

// 完成队列条目 (CQE)
struct io_uring_cqe {
    __u64 user_data;  // 从 SQE 传回
    __s32 res;        // 结果码 (字节数或 -errno)
    __u32 flags;
};
```

### 三种运行模式

| 模式 | 系统调用次数 | 说明 |
|------|-------------|------|
| **基本模式** | 每批 1 次 `io_uring_enter()` | 用户填充 SQE 后调用 enter 提交 |
| **SQPOLL 模式** | 0 次 (内核线程轮询 SQ) | 内核线程自动轮询提交队列，完全无系统调用 |
| **IOPOLL 模式** | 轮询 CQ | 适用于 NVMe 轮询 I/O，无中断开销 |

### 支持的操作 (6.x)

| 操作 | 说明 |
|------|------|
| `IORING_OP_READV` / `WRITEV` | 向量读/写 (支持缓冲 I/O) |
| `IORING_OP_READ_FIXED` / `WRITE_FIXED` | 预注册缓冲区读/写 |
| `IORING_OP_OPENAT` / `CLOSE` | 打开/关闭文件 |
| `IORING_OP_STATX` | 获取文件元数据 |
| `IORING_OP_ACCEPT` / `CONNECT` | 网络连接 |
| `IORING_OP_SEND` / `RECV` | 网络收发 |
| `IORING_OP_EPOLL_CTL` | epoll 控制 |
| `IORING_OP_TIMEOUT` | 超时控制 |

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 |
|-----------|-------------|
| AIO (`io_submit` / `io_getevents`) | io_uring (`io_uring_setup` / `io_uring_enter`) |
| 只支持 O_DIRECT | 支持缓冲 I/O + O_DIRECT |
| 每次操作需系统调用 | SQPOLL 模式零系统调用 |
| 只支持 read/write | 支持 open/stat/accept/send/recv/... |
| 接口复杂 (iocb 结构) | liburing 简化 API |

### liburing 简化 API

```c
// 使用 liburing 库 (推荐)
struct io_uring ring;
io_uring_queue_init(32, &ring, 0);

struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
io_uring_prep_readv(sqe, fd, &iov, 1, offset);
io_uring_submit(&ring);

struct io_uring_cqe *cqe;
io_uring_wait_cqe(&ring, &cqe);
printf("read %d bytes\n", cqe->res);
io_uring_cqe_seen(&ring, cqe);
```

---

## HFT 关联

| 场景 | io_uring 优势 |
|------|--------------|
| **异步日志** | 非阻塞写入日志文件，不阻塞交易线程 |
| **网络 I/O** | 替代 epoll + read/write，减少系统调用 |
| **零拷贝** | 预注册缓冲区 (IORING_REGISTER_BUFFERS) 避免 get_user_pages |
| **批量提交** | 一次 io_uring_enter 提交多个 I/O，减少上下文切换 |

> **HFT 实盘：** io_uring 的 SQPOLL 模式对 HFT 日志写入极有价值——交易线程将日志条目写入共享内存 SQ，内核线程异步刷盘，交易线程零阻塞。网络 I/O 方面，io_uring 可替代 epoll，在高连接数场景下减少系统调用开销。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** io_uring 的 SQ 和 CQ 为什么用环形缓冲区而不是链表？

> 环形缓冲区是固定大小的共享内存，用户态和内核通过 head/tail 指针通信，不需要锁（单生产者单消费者模型）。链表需要动态分配内存和锁保护。环形缓冲区对 cache 友好，且 mmap 共享后无需数据拷贝。

**Q2:** SQPOLL 模式如何实现零系统调用？

> SQPOLL 启动一个内核线程持续轮询提交队列 (SQ)。用户态写 SQE 到 SQ 后不需要调用 `io_uring_enter()`，内核线程会自动发现并提交。只有当内核线程空闲超过 1 秒停止后，才需要 `io_uring_enter()` 重新唤醒。代价是内核线程持续消耗 CPU。

**Q3:** io_uring 为什么支持缓冲 I/O 的异步操作，而 AIO 不支持？

> AIO 设计上要求 O_DIRECT，因为缓冲 I/O 可能在页缓存命中时立即完成（不需要异步）。但 AIO 的接口不允许这种情况，导致缓冲 I/O 会阻塞。io_uring 通过 CQE 通知完成，即使页缓存命中也能异步返回结果（通过 CQE 而非阻塞调用者）。

</details>
