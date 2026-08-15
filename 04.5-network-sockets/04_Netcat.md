# 04 · Netcat（nc）

<a id="pnp-04-goal"></a>

## 目标

最小 **双向字节管道**：stdin ↔ TCP ↔ 对端；理解半关闭、`shutdown`、两端同时读写。

<a id="pnp-04-unp"></a>

## UNP 对照

- Ch5 并发 echo、Ch6 I/O 多路复用、Ch14 高级 I/O

<a id="pnp-04-concepts"></a>

## 概念详解

### 1. Netcat 是什么：两个 fd 之间的泵

```
stdin ──read──> 缓冲 ──write──> socket ──网络──> 对端 socket ──> stdout
```

一句话：把任意的标准输入输出接到 TCP 上。它是网络调试的"万用表"：手工发协议帧、探测端口（`nc -z host port`）、传文件（`nc -l 9000 > file` + `nc host 9000 < file`）。

### 2. 阻塞模型的根本困境

最朴素的实现：

```c
while ((n = read(STDIN_FILENO, buf, sizeof buf)) > 0)
    writen(sockfd, buf, n);
```

问题：`read(stdin)` **阻塞** 期间，socket 上到达的数据没人读——对端等你回复，你等键盘输入，**互相等死**。三种解法：

| 方案 | 思路 | 代价 |
|------|------|------|
| 双进程/线程 | 各管一个方向 | 进程/线程同步、退出协调复杂 |
| `select`/`poll` | 同时监听 stdin + socket | 单线程事件驱动（nc 与 muduo 的选择） |
| 非阻塞轮询 | 循环试两个 fd | 忙等烧 CPU（见 [06](./06_NonBlockingIO.md)） |

这是 **事件驱动模型的第一课**：不止 socket，任何阻塞 fd（tty、pipe、文件描述符）都可以进 `select`/`epoll`。

### 3. `close` vs `shutdown`：半关闭是核心区别

| | `close(fd)` | `shutdown(fd, how)` |
|---|-------------|----------------------|
| 语义 | 关闭该 fd（引用计数归零才发 FIN） | 只控制连接方向，不关 fd |
| `SHUT_WR` | — | 发 FIN，**之后仍可 `read`**（对端 EOF 方向关闭，自己收方向保留） |
| `SHUT_RD` | — | 关读方向（后续 read 行为平台有差异） |
| `SHUT_RDWR` | — | 两方向都关，但不释放 fd |
| 多进程 fork 后 | 计数未归零 → FIN 拖着不发 | 立即生效 |

典型协议用法："客户端发完请求 → `shutdown(SHUT_WR)` → 继续读响应"。对端 `read` 返回 0（EOF），明确知道"不会再有数据"，但它的发送方向还开着——这就是 **优雅关闭（graceful shutdown）** 的基础。

<a id="pnp-04-code"></a>

## C++ 示例：select 版最小 nc（客户端方向）

```cpp
// mini_nc.cpp — g++ -O2 mini_nc.cpp -o mnc ; ./mnc 127.0.0.1 9000
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(9000);
    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }

    fd_set rset;
    int    maxfd1 = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
    bool   stdin_eof = false;
    char   buf[4096];

    for (;;) {
        FD_ZERO(&rset);
        if (!stdin_eof) FD_SET(STDIN_FILENO, &rset);
        FD_SET(sockfd, &rset);
        int n = select(maxfd1 + 1, &rset, nullptr, nullptr, nullptr);
        if (n < 0) { if (errno == EINTR) continue; perror("select"); break; }

        if (FD_ISSET(sockfd, &rset)) {           // socket 可读
            ssize_t r = read(sockfd, buf, sizeof(buf));
            if (r == 0) { puts("\n[peer closed]"); break; }   // 收到 EOF
            if (r < 0) { perror("read sock"); break; }
            write(STDOUT_FILENO, buf, r);
        }
        if (!stdin_eof && FD_ISSET(STDIN_FILENO, &rset)) {    // 键盘可读
            ssize_t r = read(STDIN_FILENO, buf, sizeof(buf));
            if (r == 0) {                      // 本端输入结束
                stdin_eof = true;
                shutdown(sockfd, SHUT_WR);     // 半关闭：发 FIN，但继续收
                continue;
            }
            ssize_t w = write(sockfd, buf, r);  // 简化：未处理部分写
            (void)w;
        }
    }
    close(sockfd);
    return 0;
}
```

注意 stdin EOF 后：从 `select` 集合里摘掉 stdin（否则它永久可读 → **busy loop**），但保留 socket 继续收对端剩余数据。muduo 对应 `TcpConnection::shutdown()`（关写方向）与 `forceClose()`（RST 硬断）的区别。

<a id="pnp-04-run"></a>

## Rust 运行（上游已有实现）

```bash
cd ~/Desktop/"Computer Networking"/PNP/code/04_Netcat/rewrite_rust

# 监听（单连接）
cargo run -- -l 9000

# 另一终端连接
cargo run -- 127.0.0.1 9000
```

<a id="pnp-04-kernel"></a>

## 内核视角

- `shutdown(SHUT_WR)` → `tcp_shutdown()` 立刻在发送队列尾构造 FIN 报文（不等未确认数据，FIN 排在其后），连接进入 `FIN_WAIT_1`；对端内核回 ACK，本端进入 `FIN_WAIT_2`，**接收方向照常工作**
- `close()` 则要等发送缓冲数据全部送达（`linger` 未设置时内核"尽力"后台发送）才发 FIN，且 fd 引用计数为 0 才触发
- 对端 `read` 返回 0 的判定：收到 FIN 只是标记 `sk_shutdown |= RCV_SHUTDOWN`，`tcp_recvmsg` 发现队列空且有该标记才返回 0
- RST 的产生：收到 FIN 后仍向对端 `write` → 对端回 RST；`setsockopt(SO_LINGER{1,0})` + close → 直接 RST。**收到 RST 后本端再 read/write 直接报 `ECONNRESET`**

状态机全程可用 `ss -tn` 观察各状态（`FIN_WAIT2`/`CLOSE_WAIT` 停留过长 = 应用层没做半关闭或没 close，见 [12 TCP/IP](../12-tcpip-protocols/) 的连接管理章节）。

<a id="pnp-04-pitfalls"></a>

## 坑点

- 只关写端 vs `close` 整连接（对端 `read` 得 0）
- 单线程阻塞：一端堵住另一端（课程常引到非阻塞 / epoll）
- Windows 与 POSIX 行为差异（本 Rust 版用 `std::net`，跨平台）
- `CLOSE_WAIT` 堆积：对端发了 FIN 而本端应用从不 `close` ——泄漏的是代码逻辑不是 fd
- fork 出的子进程继承 socket fd：`close` 不发 FIN（引用计数），必须 `shutdown` 或 `SOCK_CLOEXEC`
- 把 `read` 返回 0 当错误处理（EOF 是正常语义）

<a id="pnp-04-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 会话拆除 | 撮合网关要求 **先发注销/心跳停止，再 shutdown 写方向，读完剩余回报，最后 close**——次序错了会丢最后一批成交确认 |
| `CLOSE_WAIT` 告警 | 网关连接数缓慢上涨的常见根因，运维信号（配合 [16 BPF 观测](../16-bpf-observability/)） |
| 手工调试 | 用 nc 向行情网关发原始二进制帧复现问题，比写测试程序快 |
| FIN/RST 延迟 | RST 语义粗暴且可能丢弃对端未读数据——交易链路永远走优雅关闭 |

<a id="pnp-04-quiz"></a>

## 自测题

1. `shutdown(fd, SHUT_WR)` 之后 `read` 还能用吗？`write` 呢？
2. fork 出的子进程里 `close(sockfd)`，父进程还在用——对端会收到 FIN 吗？为什么？
3. `select` 返回后不处理 stdin EOF（不从集合摘除）会发生什么？
4. `CLOSE_WAIT` 大量出现，是本端 bug 还是对端 bug？
5. 收到 RST 与收到 FIN，本端 `read` 的表现分别是什么？

<a id="pnp-04-refs"></a>

## 交叉引用

- 上一篇：[03 自连接](./03_SelfConnect.md) · 下一篇：[05 TTCP](./05_TTCP.md)
- [06 非阻塞 I/O](./06_NonBlockingIO.md) · [07 epoll](./07_IO_epoll.md) · [12 TCP/IP 协议](../12-tcpip-protocols/) · [12.5 Wireshark](../12.5-wireshark-packet-analysis/)
