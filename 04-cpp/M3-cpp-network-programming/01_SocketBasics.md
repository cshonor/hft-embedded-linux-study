# 01 · Socket 基础

<a id="pnp-01-goal"></a>

## 目标

字节序、`sockaddr`、`connect`/`bind`；用 Daytime 跑通 TCP 客户端/服务器。

<a id="pnp-01-unp"></a>

## UNP 对照

| 内容 | 链接 |
|------|------|
| 客户端 | [UNP 1.2](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.2_SimpleTimeClient.md) |
| 服务器 | [UNP 1.5](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.5_SimpleTimeServer.md) |
| C/S 联合 | [1.12 小结](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.12_Summary.md) |

<a id="pnp-01-concepts"></a>

## 概念详解

### 1. 字节序：为什么端口和 IP 要 `htons`/`htonl`

- x86/ARM 都是 **小端**（低字节在低地址），网络字节序是 **大端**。
- `sin_port`、`sin_addr` 在协议头部按大端解释——直接把主机 `int` 塞进去，抓包看到的就是"换了个端口"。
- 记忆：**进网络前转一次，出网络后转一次**，其余代码一律用主机序。

```c
#include <netinet/in.h>
uint16_t net_port = htons(10013);       // host -> network (16-bit)
uint32_t net_ip   = htonl(INADDR_ANY);  // 0.0.0.0，监听所有网卡
```

### 2. `sockaddr` 家族：为什么都要 cast 成 `sockaddr*`

```c
struct sockaddr_in {            // AF_INET
    sa_family_t    sin_family;  // 地址族，必须与 socket() 第一个参数一致
    in_port_t      sin_port;    // 网络字节序
    struct in_addr sin_addr;    // 4 字节 IP
    char           sin_zero[8]; // 填充，让两种结构体等长
};
```

`bind/listen/connect/accept` 的签名只认通用的 `struct sockaddr*`（内核态里按 `sa_family` 再解释回来）。这个 cast 是 C 时代"穷人的继承"，**C++ 不需要你写得更优雅——这套 API 就是 C 接口**，理解它是为了看懂 muduo 封装的价值。

### 3. 五个系统调用各自的职责

| 调用 | 职责 | 内核侧发生什么 |
|------|------|----------------|
| `socket()` | 创建通信端点，返回 fd | 分配 `struct file`（VFS）→ `struct socket`（BSD 层）→ `struct tcp_sock`（inet 层） |
| `bind()` | 绑定本地 IP:port | 把四元组的"本地半边"写进 socket；未 bind 时 connect/发送会自动选 |
| `listen()` | 被动打开，开始收 SYN | 状态 `CLOSED→LISTEN`；`backlog` 限定 **完成三次握手的连接队列** 长度 |
| `connect()` | 主动打开 | 发 SYN，进程阻塞在 `SYN_SENT`，收到 SYN+ACK 才返回 |
| `accept()` | 从队列取一个已建连 | 返回 **新的 fd**（connfd），listenfd 继续收新连接 |

关键认知：**三次握手发生在 `accept` 返回之前**（由内核协议栈完成）。`accept` 只是"从成品队列里取货"——这是后面理解 SYN flood、`TCP_DEFER_ACCEPT` 的基础。

### 4. `accept` 返回新 fd，不是复用 listenfd

`listenfd` 是"前台接待"，每个 `accept` 出来的 `connfd` 是一个独立的四元组连接。关闭顺序错了（坑点 3）会把接待窗口砸了。

<a id="pnp-01-code"></a>

## C++ 示例：Daytime 服务器 + 客户端

端口用 10013（非特权，不需要 root；特权端口 <1024 见坑点）。

```cpp
// daytime_server.cpp — g++ -O2 -Wall daytime_server.cpp -o dtserver
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <ctime>

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listenfd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 监听 0.0.0.0
    addr.sin_port        = htons(10013);

    if (bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(listenfd, 5);   // accept 队列容量（受 somaxconn 上限约束）

    for (;;) {
        sockaddr_in cli{}; socklen_t len = sizeof(cli);
        int connfd = accept4(listenfd, reinterpret_cast<sockaddr*>(&cli), &len,
                             SOCK_CLOEXEC);
        if (connfd < 0) { perror("accept"); continue; }

        time_t t = time(nullptr);
        std::string msg = ctime(&t);          // 自带 '\n'
        write(connfd, msg.data(), msg.size()); // 注：ctime 返回 char*，此处简化
        close(connfd);                         // 短连接：发完即关
    }
}
```

```cpp
// daytime_client.cpp — g++ -O2 -Wall daytime_client.cpp -o dtclient
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(10013);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);   // 点分十进制 -> 4 字节

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }

    char buf[128];
    ssize_t n;
    // read 必须循环：TCP 是字节流，一次 read 不保证收全（见 02）
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, stdout);
    }
    close(fd);
}
```

<a id="pnp-01-kernel"></a>

## 内核视角

- **fd 三层结构**：`fd`（进程 fd 表索引）→ `struct file`（VFS 通用层）→ `struct socket` → `struct tcp_sock`。`write()` 走 VFS `file_operations` 分发到 `sock_write_iter`，最终进 TCP 发送队列（`sk_write_queue`，元素是 `sk_buff`）。这条链路在 [05-linux-kernel](../../05-linux-kernel/) 和 [12-kernel-networking](../../12-kernel-networking/) 里逐层展开。
- **backlog 语义（Linux 2.2+）**：`listen(fd, backlog)` 只控制 accept 队列；半连接（SYN 收到未握手完成）队列由 `tcp_max_syn_backlog` 控制，且受 SYN cookie 影响。accept 队列满了，内核会对新完成的握手 **丢弃 ACK 或拒绝**（`tcp_abort_on_overflow=1` 时直接 RST）——客户端看到"连接成功又立刻被断"的经典谜题。
- **队列上限**：`backlog` 实际取 `min(backlog, net.core.somaxconn)`，默认 somaxconn=128（旧内核），调优时要两边一起改。

<a id="pnp-01-pitfalls"></a>

## 坑点（PNP 常强调）

- `sin_addr` = IP，`sin_port` = 端口（`htons`）——两个字段搞反/漏转是第一天的经典 bug
- 特权端口 13：非 root 可改 10013 做实验
- `read` 须循环直到 FIN（见 [02 粘包](./02_TCPByteStream.md)）
- **`SIGPIPE` 默认杀死进程**：对端已关闭后再 `write`，内核发 SIGPIPE。服务端必须 `signal(SIGPIPE, SIG_IGN)`（muduo 在初始化时全局忽略）或用 `send(..., MSG_NOSIGNAL)`
- **忘记 `close(connfd)`** → fd 泄漏，进程悄悄逼近 `ulimit -n`，然后 `accept` 返回 `EMFILE` 且不分配 fd——必须在事件循环里特殊处理（见 [06](./06_NonBlockingIO.md)）
- `inet_addr()` 已废弃（`INADDR_NONE` 与 `255.255.255.255` 冲突），用 `inet_pton`
- `connect` 被信号中断返回 `EINTR`，但连接 **可能仍在进行**——不能简单重试（见 UNP Ch4 中断的 connect）

<a id="pnp-01-hft"></a>

## HFT 关联

| 场景 | 与本节的关系 |
|------|--------------|
| 连接预热 | 交易会话在开盘前建立（避开关键路径上的三次握手 RTT），空闲时发心跳保活 |
| 建连延迟 | `connect` 阻塞一个 RTT；低延迟场景研究 `TCP_FASTOPEN`（0-RTT 数据）或干脆 UDP/组播 |
| fd 上限 | HFT 网关维护数百条行情/订单连接，`ulimit -n` 与 `somaxconn` 是上线 checklist 项 |
| 抓包验证 | 建连阶段是 [12.5 Wireshark](../../11.5-wireshark-packet-analysis/) 的第一个实验对象 |

<a id="pnp-01-quiz"></a>

## 自测题

1. `accept` 返回的 connfd 和 listenfd 是什么关系？关掉 listenfd，已建立的连接会怎样？
2. `listen(fd, 512)` 就能让 accept 队列到 512 吗？还要改什么？
3. 客户端 `connect` 成功返回时，服务器可能还没调用 `accept`——为什么？
4. 为什么 `write` 一个对端已关闭的连接不会返回错误而是收到 SIGPIPE？（提示：错误是 **异步** 到达的）
5. `INADDR_ANY` 和绑定具体网卡 IP 的区别？多网卡机器上行情接收该用哪个？

<details>
<summary>参考答案</summary>

1. `listenfd` 是"前台接待"：只持有四元组的**本地半边**（本机 IP:port），本身不承载任何数据，读它无意义。`accept` 从已完成握手的队列里取出一条连接，**新建一个 fd（connfd）**，它才拥有完整四元组，代表这条具体连接；listenfd 继续留着收新连接。所以一个监听端口对应 1 个 listenfd + N 个 connfd。**关掉 listenfd 不影响已建立的 connfd**（它们照常收发、照常 FIN），只是不能再 `accept` 新连接，后续 SYN 会被拒绝/丢弃；反之误关某个 connfd 只断那一条连接。注意 fork 后 `close` 只是引用计数减一，计数未归零不会发 FIN。

2. 不能。Linux 上 accept 队列的实际长度是 `min(backlog, net.core.somaxconn)`，旧内核 `somaxconn` 默认 **128**（新内核多为 4096），所以 `listen(fd, 512)` 会被截到 128。必须同时改内核：`sysctl -w net.core.somaxconn=512`（写进 `/etc/sysctl.conf` 持久化）。另外两点：glibc 的 `SOMAXCONN` 宏是老值 128，用宏也上不去；**半连接队列**是另一条队列，由 `net.ipv4.tcp_max_syn_backlog` 控制，抗 SYN flood 还要看 SYN cookie 与 `tcp_abort_on_overflow`。

3. 因为**三次握手由内核协议栈完成，不需要应用参与**：客户端发完最后一个 ACK 就进入 ESTABLISHED，服务端内核把这条连接放进 listen socket 的 accept（完成）队列，`connect` 随即成功返回。`accept` 只是"从成品队列里取货"，可能几百微秒甚至更久之后才被调用。所以 `connect` 成功只说明握手完成，**不代表对端已 `accept`，更不代表应用已处理**——数据会先堆在接收缓冲里。这是理解 SYN flood、`TCP_DEFER_ACCEPT`、自连接的前提。

4. 因为错误是**异步**到达的。第一次 `write` 只是把数据拷进发送缓冲并发出报文，此时内核还不知道对端已关闭，于是**正常返回**（返回值＝请求长度）。随后对端回一个 RST，本端收到后才把 socket 标记为错误：此后的 `write` 返回 -1/`EPIPE`，同时内核向进程发 **SIGPIPE**（默认动作是**杀死进程**）。所以"write 成功"既不代表对端收到，也不代表连接健康。对策：服务端 `signal(SIGPIPE, SIG_IGN)`（muduo 全局忽略）或 `send(..., MSG_NOSIGNAL)`，并永远检查 `write` 返回值。

5. `INADDR_ANY`（0.0.0.0）是**通配地址**：监听本机所有网卡上的该端口，无论报文从哪张卡进来都能被接受；绑定具体 IP 则只收目的地址是该 IP 的连接（其它网卡/地址的流量到达不了这个 socket）。多网卡行情接收的结论：**TCP 服务器场景通常绑具体网卡 IP**，这样每条会话明确对应一张物理卡，便于隔离行情网段、避免路由非对称和"接错网"，也允许每卡起一个实例；只有"不在乎从哪张卡来、要全覆盖"时才用 `INADDR_ANY`。若是 **UDP 组播**接收，推荐直接 `bind` 到组播地址 + 端口（绑 `INADDR_ANY` 也能收，但会顺带收到该端口上的单播流量），并用 `imr_interface` 指定从哪张卡入组。

</details>

<a id="pnp-01-refs"></a>

## 交叉引用

- 下一篇：[02 TCP 字节流与粘包](./02_TCPByteStream.md)
- [03.5 UNP Ch1](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/study.md) · [03 Linux 用户态 API](../../03-linux-userspace-api/) · [12 TCP/IP 协议](../../11-tcpip-protocols/) · [13 内核网络](../../12-kernel-networking/)
