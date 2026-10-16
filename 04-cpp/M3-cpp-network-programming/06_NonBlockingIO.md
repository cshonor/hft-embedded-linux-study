# 06 · 非阻塞 I/O

<a id="pnp-06-goal"></a>

## 目标

`fcntl` `O_NONBLOCK`；`EAGAIN`/`EWOULDBLOCK` 不是致命错误；与阻塞模型对比。

<a id="pnp-06-unp"></a>

## UNP 对照

- [1.4 特殊 errno](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.4_ErrorHandlingWrapper.md)
- Ch16 Nonblocking I/O

<a id="pnp-06-concepts"></a>

## 概念详解

### 1. 非阻塞是什么：把"等"从内核搬回用户态

阻塞模型：`read` 没数据 → 进程睡眠（挂到 socket 等待队列，上下文切换）→ 数据到达被唤醒。
非阻塞模型：`read` 没数据 → **立刻返回 -1 + `EAGAIN`**，"现在没有，你稍后再来"。

一句话：**非阻塞把"何时有数据"的决策权交给应用**。单线程要同时伺候多个 fd（其中任何一个都可能暂时无数据），就必须非阻塞——这是 [07 epoll](./07_IO_epoll.md) 的前置条件。

### 2. 设置方法

```cpp
#include <fcntl.h>
int setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);         // 必须先读再改再写回
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

坑：`fcntl(fd, F_SETFL, O_NONBLOCK)` 不读旧值直接写——**把 fd 上其他标志（如 O_APPEND）清掉了**。新建 socket 更简单：`socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)`，一次原子完成。

### 3. 各调用在非阻塞模式下的返回

| 调用 | 无数据/不能做时 | 说明 |
|------|------------------|------|
| `read` | `-1` `EAGAIN` | 稍后再读 |
| `write` | `-1` `EAGAIN` **或部分写**（返回 < n） | 发送缓冲满；已写入的字节数必须记账 |
| `accept` | `-1` `EAGAIN` | 队列空（惊群/事件循环里常见） |
| `connect` | `-1` `EINPROGRESS` | SYN 已发出，握手 **正在进行**，不是失败！ |

`EAGAIN == EWOULDBLOCK`（Linux 上同值），语义即"再试一次"。

### 4. 非阻塞 connect：唯一"进行时"的返回值

```cpp
int r = connect(fd, ...);
if (r < 0 && errno != EINPROGRESS) { /* 真错误，如 EADDRNOTAVAIL */ }

// EINPROGRESS 之后：把 fd 加入 select/epoll 的写集合，
// 就绪后还要检查是否真的成功：
int err = 0; socklen_t len = sizeof(err);
getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
if (err) { /* connect 失败，err 是错误码，如 ECONNREFUSED */ }
```

为什么必须 `SO_ERROR`：非阻塞 connect 完成的信号是"fd 可写"，但失败（收到 RST）同样表现为可写——**可写 ≠ 成功**。

### 5. 阻塞 vs 非阻塞 vs 事件驱动

| 模型 | 单 fd 等待 | 多 fd | CPU 占用 |
|------|-----------|-------|----------|
| 阻塞 + 每连接一线程/进程 | 内核睡眠，零开销 | 线程数=连接数，内存/切换成本 | 低 |
| 非阻塞 + 轮询 | — | `for fd: try_read()` | **100% 忙等** |
| **非阻塞 + epoll** | — | 就绪回调，只处理有事件的 | 低，且单线程可承万级连接 |

muduo 的组合拳：**epoll LT + 非阻塞 fd**（Ch 6.2 陈硕的选择）——epoll 保证只碰就绪的 fd，非阻塞保证碰了也不会卡住（LT 水平触发下事件可能已被处理过，重读必须能立即返回）。

<a id="pnp-06-code"></a>

## C++ 示例：非阻塞读的完整姿势

```cpp
// 处理一次 EPOLLIN 事件（LT 模式）
void handleRead(int fd, Buffer* out) {
    for (;;) {                                  // 循环读：一次事件尽量榨干
        char buf[65536];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            out->append(buf, n);                // 先攒着，粘包解码另做（见 02）
            continue;
        }
        if (n == 0) { /* 对端 EOF，关连接 */ return; }
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 读干了：正常退出
        if (errno == EINTR) continue;
        /* 其他 errno：ECONNRESET 等 → 出错处理 */ return;
    }
}

// 非阻塞写：partial write 记账
void handleWrite(int fd, Buffer* pending) {
    while (pending->readableBytes() > 0) {
        ssize_t n = write(fd, pending->peek(), pending->readableBytes());
        if (n > 0) { pending->retrieve(n); continue; }
        if (errno == EAGAIN) {
            // 发送缓冲满：注册 EPOLLOUT，等下次可写再续（见 07）
            enableWritable(fd);
            return;
        }
        if (errno == EINTR) continue;
        return;  // 错误
    }
    disableWritable(fd);    // 全发完：立刻注销 EPOLLOUT，否则 busy loop
}
```

<a id="pnp-06-kernel"></a>

## 内核视角

- `O_NONBLOCK` 存在 `struct file->f_flags`。`tcp_recvmsg` 检查：接收队列为空时，阻塞 → 把进程挂到 `sk_wq` 等待队列并 `schedule()`；非阻塞 → 直接 `-EAGAIN` 返回，**全程零睡眠零切换**
- `tcp_sendmsg`：发送缓冲剩余空间不足以容纳全部数据时——阻塞模式等空间（可能部分写后睡）；非阻塞模式 **一点空间都没有才 EAGAIN，有一点就部分写**——所以非阻塞 write 返回正数 n < 请求值是常态
- 唤醒延迟的物理成本：数据到达 → 硬中断/软中断 → 唤醒进程 → **调度器安排它上 CPU**（µs 级，还要看负载）。这是内核阻塞/唤醒路径的固有开销，也是 HFT 后来发明 **busy polling**（`SO_BUSY_POLL`，进程自旋问内核"有了吗"）和 **内核旁路**（[13 DPDK](../../13-dpdk/)，用户态驱动直接收包）的动机
- `accept` 的 `EMFILE`：fd 用尽时 accept 会 **持续返回 EMFILE**，非阻塞事件循环若不处理（如先 reserve 一个 idle fd 应急 close），listenfd 一直可读 → busy loop 拒绝服务

<a id="pnp-06-pitfalls"></a>

## 坑点

- 非阻塞 `connect` 返回 `EINPROGRESS`
- 忙等 vs 事件驱动（见 07）
- `fcntl` 直接 `F_SETFL` 不读旧值，清掉其他标志
- 非阻塞 write 只处理 EAGAIN 不处理 **部分写**——数据悄悄丢
- 可写事件不注销 → EPOLLOUT 永远触发 → CPU 100%（LT 模式经典 bug）
- 把 `EAGAIN` 当错误断开连接
- 非阻塞 fd 交给阻塞假设的代码（如 stdio 缓冲）使用，行为混乱

<a id="pnp-06-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 热路径收包 | 订单回报到达必须立刻处理：阻塞唤醒（µs 级调度延迟）不可接受 → 自旋 `recv(MSG_DONTWAIT)`（单次调用级非阻塞，不改 fd 标志） |
| `SO_BUSY_POLL` | 内核提供的折中：进程自旋轮询驱动队列，省掉唤醒，延迟亚微秒~微秒 |
| 内核旁路 | 自旋仍要过协议栈；再往下是 [DPDK](../../13-dpdk/)（用户态轮询驱动 + 自带 TCP 栈），muduo/epoll 这套就整体让位 |
| 事件循环设计 | 网关的非热路径（管理、监控连接）继续 epoll LT + 非阻塞——工程成熟度与延迟的分层组合 |

<a id="pnp-06-quiz"></a>

## 自测题

1. 为什么 muduo 用 LT 触发也必须配非阻塞 fd？（提示：同一个 fd 被两个事件循环/重入处理时）
2. 非阻塞 `write` 返回 100（请求 1000），剩下 900 字节去哪了？谁负责？
3. 非阻塞 connect 成功/失败都表现为"可写"，怎么区分？
4. `MSG_DONTWAIT` 与 `O_NONBLOCK` 的区别？为什么热路径自旋用前者？
5. 进程被唤醒到真正跑起来，中间发生了什么？这笔开销大概是多少？

<details>
<summary>参考答案</summary>

1. LT 的含义是"只要缓冲里**还有**数据就一直报告"，这恰恰意味着**同一件事可能被处理两次**：两个事件循环/两个线程同时持有该 fd、回调里又同步跑了一次事件循环（重入）、或本轮只处理了部分数据。此时再 `read`，如果 fd 是阻塞的而数据已被前一个处理者读干，就会**阻塞整个事件循环**（所有其它连接一起卡死）。非阻塞 fd 保证"碰了也不会卡住"——读到 `EAGAIN` 就退出，最坏情况是多一次无效调用。ET 则更严格：必须循环读到 `EAGAIN`，**强制**非阻塞。

2. 剩下 900 字节**根本没进内核**（`tcp_sendmsg` 发现发送缓冲只剩 100 字节空间，拷完就返回了），也不会自动重发。责任在**应用层**：必须按返回值记账——把已发出的 100 字节从 pending buffer 取走，剩下的继续挂在输出 Buffer 上，然后注册 `EPOLLOUT`，等可写事件再续写；全部发完**立刻注销 EPOLLOUT**（LT 下不注销会 100% CPU）。muduo 的 `TcpConnection::send()` 就是这个逻辑。牢记：非阻塞 `write` 是"有一点空间就部分写"，返回 `0 < n < 请求值` 是常态，只看 `EAGAIN` 不处理部分写 = 静默丢数据。

3. 用 `getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)` 取 **pending error**：`err == 0` 即连接成功；`err != 0` 就是失败原因（`ECONNREFUSED`、`ETIMEDOUT`、`EHOSTUNREACH` 等），此时应关闭 fd 并按错误处理。原因是：非阻塞 connect **成功和失败都表现为"fd 可写"**——收到 RST 同样让 socket 变为可写——所以"可写 ≠ 成功"。辅助判据：成功后再次 `connect` 返回 `EISCONN`，且 `getpeername()` 有效。

4. `O_NONBLOCK` 是 **fd 上的持久标志**（存在 `struct file->f_flags`），影响该 fd 上**所有**读写；`MSG_DONTWAIT` 是**单次调用**的 flag，只让这一次 `recv`/`send` 非阻塞，不改 fd 状态。热路径自旋用 `MSG_DONTWAIT`：自旋循环本来就是自己反复调用 `recv`，单次非阻塞已足够；改 fd 标志反而会波及**共享同一 `struct file` 的其它路径**（dup/fork 后标志是共享的），让别处意外拿到 `EAGAIN`，也破坏"阻塞/非阻塞"约定。另外 `O_NONBLOCK` 要用 `F_GETFL`→改→`F_SETFL`，直接 `F_SETFL` 会清掉 `O_APPEND` 等标志。

5. 数据到达 → 网卡硬中断 → 软中断把 skb 放入接收队列 → `sk_data_ready` 唤醒等在 `sk_wq` 上的进程（置为 `TASK_RUNNING` 挂进 runqueue）→ **调度器择机上 CPU**（可能被更高优先级任务抢占，跨核还要等 IPI/迁移）→ 恢复上下文、cache/TLB 冷启动。这笔开销通常是 **微秒级**（几 µs，负载高或跨核时更大）。正是这笔"唤醒 + 调度"延迟，催生了 busy polling（`SO_BUSY_POLL`，进程自旋问内核"有了吗"）和内核旁路（DPDK，用户态轮询驱动直接收包）。

</details>

<a id="pnp-06-refs"></a>

## 交叉引用

- 上一篇：[05 TTCP](./05_TTCP.md) · 下一篇：[07 epoll](./07_IO_epoll.md)
- [03.5 UNP Ch16](../../03.5-unix-network-api/) · [13 内核网络](../../12-kernel-networking/) · [13 DPDK](../../13-dpdk/)
