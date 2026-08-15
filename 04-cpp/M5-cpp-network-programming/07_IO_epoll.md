# 07 · I/O 复用（select / poll / epoll）

<a id="pnp-07-goal"></a>

## 目标

**就绪**通知 vs 真正 I/O；水平触发 vs 边缘触发；单线程多连接。

<a id="pnp-07-unp"></a>

## UNP 对照

- Ch6 I/O Multiplexing、Ch16

<a id="pnp-07-concepts"></a>

## 概念详解

### 1. I/O 复用解决什么

阻塞 I/O 一次只能等一个 fd；一个连接一个线程在万级连接下（C10K）线程内存与切换成本爆炸。I/O 复用 = **一次系统调用，把 N 个 fd 的"谁就绪了"问清楚**，然后只对就绪 fd 做 I/O。

注意它复用的是 **等待**，不是 I/O 本身——`epoll_wait` 返回后 `read`/`write` 还是要自己做。就绪 ≠ 数据到手。

### 2. 三代 API 对比

| | select | poll | epoll |
|---|--------|------|-------|
| fd 数量上限 | `FD_SETSIZE`（默认 1024，编译期硬编码） | 无（`pollfd` 数组） | 无（受 `ulimit -n`） |
| 每次调用成本 | O(n) 线性扫描 + **每次全量拷贝 fd 集进内核** | 同左 | O(1)：注册一次，`epoll_wait` 只取就绪链表 |
| fd 传递 | 每次重建 fd_set | 每次传数组 | `epoll_ctl` 增量注册 |
| 触发模式 | LT | LT | **LT + ET** |
| 跨平台 | 是（Windows 也有） | POSIX | 仅 Linux（BSD: kqueue） |

select 至今的合理用途：同时等 **超时 + 少量 fd + 信号安全的简单场景**（如 nc）；连接数上量后没有理由不用 epoll。

### 3. LT vs ET：语义与纪律

- **LT（水平触发，默认）**：只要接收缓冲 **还有** 数据，每次 `epoll_wait` 都报告 EPOLLIN。忘了读？下次还会提醒你——宽容。
- **ET（边缘触发）**：仅在状态 **变化**（无→有）时报告一次。**必须一次读到 `EAGAIN`**（配合非阻塞 fd，循环 read 直到读干），否则残留数据再也不会有通知——连接"假死"。

| | LT | ET |
|---|----|----|
| 读法 | 一次事件读一次也行（下次还会提醒） | **必须循环到 EAGAIN** |
| fd 要求 | 非阻塞（陈硕建议，防重入卡死） | **强制** 非阻塞 |
| epoll_wait 返回次数 | 多 | 少一点 |
| 编程复杂度 | 低，不易出 bug | 高（漏读=死连接，EPOLLOUT 注销逻辑更绕） |

muduo 明确选 **LT**（陈硕《Linux 多线程服务端编程》6.2）：性能差距微小，正确性差距巨大——生产系统的工程判断。

### 4. epoll 三件套

```c
int epfd = epoll_create1(EPOLL_CLOEXEC);        // 内核里建一个 eventpoll
struct epoll_event ev{ .events = EPOLLIN, .data.fd = sockfd };
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);    // 注册/修改/删除 (MOD/DEL)
struct epoll_event evs[64];
int n = epoll_wait(epfd, evs, 64, timeout_ms);  // 取就绪事件（拷回用户态）
```

常用事件：`EPOLLIN`（可读）/ `EPOLLOUT`（可写）/ `EPOLLRDHUP`（对端关写）/ `EPOLLERR`（出错，**epoll 总会报告，无需注册**）/ `EPOLLET`（边沿）/ `EPOLLONESHOT`（一次性，多线程 worker 场景防两个线程同时处理同一 fd）。

### 5. epoll 高效的原理（不是"内存拷贝少"这么简单）

- 内核 `eventpoll` 结构里有 **红黑树**（注册的所有 fd）+ **就绪链表 rdllist**
- `epoll_ctl(ADD)` 时，在目标 socket 的等待队列上注册一个 **回调**；数据到达（软中断上下文）→ 回调把该 epitem 挂进 rdllist → 唤醒等在 `epoll_wait` 上的进程
- 所以 `epoll_wait` 不需要扫描任何 fd 集合：**事件发生时已经"排队"，等待者只是取号**

对比 select：每次调用把 fd 集合整个拷进内核，内核线性遍历所有 fd 调用其 poll 方法——fd 越多越慢，且这个成本 **每个连接每次循环都付一遍**。

<a id="pnp-07-code"></a>

## C++ 示例：epoll LT echo 服务器（完整骨架）

```cpp
// echo_epoll.cpp — g++ -O2 echo_epoll.cpp -o echo_epoll
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <vector>

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));   // 重启立绑
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9007);
    bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(listenfd, SOMAXCONN);

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_event ev{};
    ev.events = EPOLLIN;                       // listenfd：LT 足矣
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    std::vector<epoll_event> evs(64);
    for (;;) {
        int n = epoll_wait(epfd, evs.data(), (int)evs.size(), -1);
        if (n < 0) { if (errno == EINTR) continue; perror("epoll_wait"); break; }

        for (int i = 0; i < n; ++i) {
            int fd = evs[i].data.fd;
            if (fd == listenfd) {                                  // 新连接
                for (;;) {                                         // LT 下读一次也行；循环收干净
                    int cfd = accept4(listenfd, nullptr, nullptr,
                                      SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (cfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        if (errno == EINTR) continue;
                        if (errno == EMFILE) { /* 应急 fd 技巧，见 06 */ break; }
                        break;
                    }
                    epoll_event cev{};
                    cev.events = EPOLLIN | EPOLLRDHUP;
                    cev.data.fd = cfd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev);
                }
            } else {                                               // 已建连接
                if (evs[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    continue;
                }
                if (evs[i].events & EPOLLIN) {
                    char buf[4096];
                    ssize_t r = read(fd, buf, sizeof(buf));         // LT：读一次
                    if (r > 0) {
                        ssize_t w = write(fd, buf, r);              // echo 简化：
                        (void)w;                                    // 完整版须处理 EAGAIN
                    } else if (r == 0 || errno != EAGAIN) {         // EOF 或真错误
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                    }
                }
            }
        }
    }
    return 0;
}
```

### muduo 对应关系（读 muduo 源码的地图）

| 本例元素 | muduo 组件 |
|----------|-----------|
| `epfd` + epoll_ctl/wait 封装 | `EPollPoller`（实现 `Poller` 接口，另有 `PollPoller`） |
| `epoll_event.data` 换成指针 | `Channel`（每个 fd 一个，记关心的事件与回调） |
| 事件循环 + `epoll_wait` | `EventLoop`（one loop per thread 的"loop"） |
| `handleRead` 回调 | `TcpConnection` 注册的 `messageCallback` 等 |
| 新连接分发 | `Acceptor` → `TcpServer` 新建 `TcpConnection` |
| 非阻塞 + LT | muduo 全局约定（`sockets::setNonBlockAndCloseOnExec`） |

<a id="pnp-07-kernel"></a>

## 内核视角

- `epoll_ctl(ADD)` → `ep_insert()`：epitem 插入红黑树，并在目标文件的等待队列（`sk_wq`）上挂 `ep_poll_callback`
- 数据到达路径：网卡中断 → softirq `tcp_v4_rcv` → 数据入接收队列 → **`sk->sk_data_ready` → ep_poll_callback → epitem 链入 rdllist → 唤醒 epoll_wait**。回调发生在软中断上下文，这就是"就绪通知是异步埋好的"的确切含义
- `epoll_wait` → `ep_poll()`：rdllist 非空则把事件 `copy_to_user`；为空则挂到 eventpoll 的等待队列上睡（可被超时/信号打断）
- **惊群**：多进程/线程等同一 epfd 或同一 listenfd，事件到来全员被唤醒却只有一个抢到——accept 用 `EPOLLEXCLUSIVE` 或 `SO_REUSEPORT` 分片缓解
- fd 关闭与 epoll 的联动：close 使 epitem 引用归零自动注销；**但 dup/fork 产生的引用会让通知照发**——事件循环里收到已 close fd 的事件九成源于此

<a id="pnp-07-pitfalls"></a>

## 坑点

- ET 必须读到 `EAGAIN`
- `listenfd` 与 `connfd` 同处 epoll 的写法
- `select` 的 fd 上限与 `FD_SETSIZE`
- LT 下注册 `EPOLLOUT` 后不注销 → 可写事件风暴（发送缓冲几乎总是可写）
- ET + 阻塞 fd：循环 read 最后一次阻塞死——ET 强制非阻塞
- `epoll_event.data` 存 fd 号 vs 存指针：fd 被关闭复用后旧事件拿 fd 号查表 → 错乱；muduo 存 `Channel*` 并用 `isNoneEvent` 防御
- 忘了 `EPOLLRDHUP`：对端半关闭时收不到通知，等 read 才发现

<a id="pnp-07-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 行情网关 | 多路行情源 + 订单网关 + 管理端口，单线程 epoll 收敛所有连接（muduo 式）已是基线架构 |
| 延迟预算 | `epoll_wait` 唤醒 + 调度 ≈ 微秒级——热路径依旧嫌慢 → [06](./06_NonBlockingIO.md) 的 busy poll → [14 DPDK](../../14-dpdk/) 内核旁路 |
| `SO_REUSEPORT` | 多核各自持独立 epoll，连接按哈希分片到核，消除锁与惊群——低延迟网关标配 |
| 观测 | 每秒事件数、事件循环延迟分布用 [16 BPF](../../16-bpf-observability/) 直接量 |

<a id="pnp-07-quiz"></a>

## 自测题

1. epoll 为什么快？把"红黑树、就绪链表、回调"串成数据到达后的一条完整路径。
2. ET 模式下读到 3000 字节返回（缓冲还有 500 字节）没继续读，会发生什么？
3. 为什么 muduo 选 LT？ET 的性能优势到底在哪个环节？
4. `EPOLLOUT` 应该常驻注册还是按需注册？为什么？
5. `epoll_wait` 返回的 fd 已被别的分支 close 了，如何防御？

<a id="pnp-07-refs"></a>

## 交叉引用

- 上一篇：[06 非阻塞 I/O](./06_NonBlockingIO.md) · 下一篇：[08 UDP/组播](./08_UDP_Multicast.md)
- [04 Netcat（select 版）](./04_Netcat.md) · [13 内核网络](../../13-kernel-networking/) · [13.5 现代网络](../../13.5-modern-networking/) · [14 DPDK](../../14-dpdk/)
