# 03 · 自连接（Self-Connect）

<a id="pnp-03-goal"></a>

## 目标

同一主机上客户端连本机监听端口时，可能出现 **自连接**；分清 `listenfd` 与 `connfd`，`accept` 返回谁。

<a id="pnp-03-unp"></a>

## UNP 对照

- [1.5 listen 队列](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.5_Appendix_listen队列.md)
- Ch4 `accept` / `connect`

<a id="pnp-03-concepts"></a>

## 概念详解

### 1. TCP 连接的唯一身份：四元组

```
(本地IP, 本地port, 对端IP, 对端port)
```

内核用四元组在哈希表（`ehash`）里唯一定位一条 TCP 连接。**四元组相同 = 同一条连接**，这是自连接问题的根源。

### 2. 自连接怎么发生的

正常流程：客户端 `connect(127.0.0.1:10013)`，内核自动分配一个临时端口（ephemeral port，范围 `net.ipv4.ip_local_port_range`，默认约 32768-60999），四元组是 `(127.0.0.1, 临时端口, 127.0.0.1, 10013)`，与监听 socket `accept` 出来的 connfd 各占一端。

**极端情况**：端口分配器恰好选中 **10013** 作为临时端口（前提：你的监听绑定允许、该四元组空闲）。此时：

```
(127.0.0.1, 10013, 127.0.0.1, 10013)   ← 四元组两端完全相同！
```

SYN 发给自己，自己回 SYN+ACK 给自己，三次握手"自言自语"完成——得到一条 **一端连着自己的连接**：

- `connect` 返回成功（它以为连上了服务器）
- 服务器 `accept` **永远收不到这条连接**（它不在监听的匹配规则内）
- 客户端 `getsockname()` == `getpeername()`（源=目的）

概率很低（单次连接万分之一量级），但两类系统会真实踩中：**压测工具疯狂建连**（端口穷尽后重用）和 **重启后立即连固定端口的服务**。

### 3. `listenfd` vs `connfd`：谁是谁

```
listenfd ──accept()──> connfd（新 fd，代表某个具体客户端）
```

| | listenfd | connfd |
|---|----------|--------|
| 数量 | 每个监听端口 1 个 | 每条连接 1 个 |
| 生命周期 | 服务器全程 | 连接期间 |
| 读 | 无意义 | 收数据 |
| 关闭后果 | 无法再接受新连接（已建连不受影响） | 断开该客户端 |

自连接的诡异表现：你 `close` 的"客户端 socket"和某条"服务端连接"其实是 **同一个四元组的两端**，一边的操作直接影响另一边。

<a id="pnp-03-code"></a>

## C++ 实验：检测自连接

```cpp
// selfconnect_probe.cpp — 不断连接本机端口，撞出四元组两端相同的情形
// g++ -O2 selfconnect_probe.cpp -o probe
// 前置：另一终端起 dtserver（见 01）
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

int main() {
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(10013);
    inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr);

    for (int i = 0; i < 200000; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (connect(fd, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) < 0) {
            perror("connect"); close(fd); continue;
        }
        sockaddr_in self{}, peer{};
        socklen_t l1 = sizeof(self), l2 = sizeof(peer);
        getsockname(fd, reinterpret_cast<sockaddr*>(&self), &l1);
        getpeername(fd, reinterpret_cast<sockaddr*>(&peer), &l2);

        char s[64], p[64];
        inet_ntop(AF_INET, &self.sin_addr, s, sizeof(s));
        inet_ntop(AF_INET, &peer.sin_addr, p, sizeof(p));
        if (self.sin_port == peer.sin_port && strcmp(s, p) == 0) {
            printf("SELF-CONNECT hit at #%d: %s:%u <-> %s:%u\n",
                   i, s, ntohs(self.sin_port), p, ntohs(peer.sin_port));
            close(fd);
            return 0;                      // 撞到了
        }
        close(fd);
    }
    puts("no self-connect in 200k tries (normal; shrink ip_local_port_range to force)");
    return 0;
}
```

复现技巧：把临时端口范围改小撞概率会大幅上升（`sysctl net.ipv4.ip_local_port_range="10013 10050"` 实验后记得改回）。

排查现成工具：`ss -tn` 看到形如 `127.0.0.1:10013 127.0.0.1:10013` 的行就是自连接。

<a id="pnp-03-kernel"></a>

## 内核视角

- `connect()` 里内核做两件事：`inet_hash_connect()` 选临时端口（尽量避开已用），然后发 SYN。选端口时只检查四元组冲突——四元组 `(127.0.0.1,10013,127.0.0.1,10013)` 若空闲就"合法"
- SYN 到达本机回环路径，`tcp_v4_rcv` 找到这条已存在的 `SYN_SENT` socket 且正好能配对 → 握手自洽完成（`tcp_rcv_state_process` 处理"自己发给自己的 SYN+ACK"）
- 内核其实有部分防护（`tcp_tw_reuse`、地址检查），但没有（也无法廉价地）完全禁止自连接——**应用层识别是最后一道防线**：`connect` 成功后比对 `getsockname`/`getpeername`，相同即断开重连

<a id="pnp-03-pitfalls"></a>

## 坑点

- 关闭错 fd 导致监听套接字被关
- 自连接时 `getpeername` / `getsockname` 表现反常（实验验证）
- 压测机和服务端同机部署：自连接概率被"同机 + 高频建连"放大
- 客户端 `bind` 固定源端口 + `SO_REUSEADDR`：四元组冲突概率人为提高，需意识到代价
- 服务端日志里"connect 成功但 accept 没见到"的连接，先怀疑自连接/端口重用，再怀疑代码 bug

<a id="pnp-03-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 会话身份 | 行情/订单会话管理以四元组为主键；自连接、端口重用会破坏"源端口=会话"的假设 |
| 压测纪律 | 延迟压测与被测服务 **分机部署**，否则自连接 + 回环带宽失真（回环不经过真实网卡栈，数据见 [13-kernel-networking](../../13-kernel-networking/)） |
| 断线重连风暴 | 重连风暴下临时端口耗尽 → `connect` 返回 `EADDRNOTAVAIL`，退避策略必须考虑端口资源 |

<a id="pnp-03-quiz"></a>

## 自测题

1. 四元组相同为什么就判定是同一条连接？内核靠哪个数据结构定位？
2. 自连接的客户端 `read` 自己 `write` 的数据会发生什么？（自己实验：会收到自己发的字节）
3. 为什么 `accept` 收不到自连接？监听 socket 匹配的是四元组的哪一半？
4. 如何用 `ss`/`netstat` 一条命令找出当前系统里的自连接？
5. `ip_local_port_range` 用尽时 `connect` 报什么错？

<a id="pnp-03-refs"></a>

## 交叉引用

- 上一篇：[02 粘包](./02_TCPByteStream.md) · 下一篇：[04 Netcat](./04_Netcat.md)
- [03.5 UNP Ch1.5 listen 队列](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.5_Appendix_listen队列.md) · [12 TCP/IP 协议](../../12-tcpip-protocols/)
