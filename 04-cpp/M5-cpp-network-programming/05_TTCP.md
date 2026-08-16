# 05 · TTCP（吞吐测试）

<a id="pnp-05-goal"></a>

## 目标

测量 TCP 吞吐；批量 `read`/`write`、禁用 Nagle、窗口与缓冲调优。

<a id="pnp-05-unp"></a>

## UNP 对照

- Ch3.9 `readn`/`writen`、Ch14 高级 I/O

<a id="pnp-05-concepts"></a>

## 概念详解

### 1. TTCP 是什么

经典的 TCP 吞吐基准（源自 BSD test TCP）：客户端发 `NBLOCKS` 个 `BUFSIZE` 的块，先发会话头（块数+块大小），发完发长度为 0 的结束标记，等服务端回一个确认（总字节数 + 总耗时），两端各自打印吞吐。

```
client: [header: nblocks,bufsize][block × N][len=0 结束标记] ──> server: 读全部，回 [acks]
```

它的价值在于把"感觉快"变成可复现的数字，以及暴露下面所有调优点。

### 2. 缓冲大小 vs 系统调用次数：第一性能杠杆

传 1 GiB，不同用户态缓冲的 `write` 次数：

| 缓冲大小 | write 次数 | 说明 |
|----------|-----------|------|
| 1 KiB | 1,048,576 | 每次系统调用 ~1-2µs，光陷入内核就烧掉秒级 |
| 4 KiB | 262,144 | 默认页大小，常见起点 |
| 64 KiB | 16,384 | 推荐量级 |
| 1 MiB | 1,024 | 边际收益开始递减（拷贝成本上升） |

经验值：**8-64 KiB** 的用户态缓冲在多数场景接近最优。太小 → 系统调用开销；太大 → CPU cache 失效 + 内存压力。

### 3. 吞吐的上限由什么决定

```
吞吐 ≈ min(链路带宽, 发送窗口 / RTT)
     发送窗口 ≈ min(cwnd, rwnd, sk_sndbuf)
```

- **RTT 大 + 窗口小** → 吞吐被窗口卡死（带宽时延积 BDP = 带宽 × RTT；跨机房 1ms × 10Gbps ≈ 1.25MB，默认 sndbuf 可能不够）
- 调大 `SO_SNDBUF`/`SO_RCVBUF` 或依赖内核自动调优（`net.ipv4.tcp_wmem`/`tcp_rmem` 的 max 值）
- `ss -ti` 现场看 `cwnd`、`snd_wnd`、`rtt`——**先看数据再调参**

### 4. Nagle 与小包

- Nagle：小于 MSS 的未确认段先攒着 → 小包场景延迟激增（与延迟确认叠加可达 40ms+）
- 吞吐测试大量写满缓冲时不触发 Nagle；但 **小块 + 请求响应** 模式必须 `TCP_NODELAY`
- 测试纪律：**测吞吐不关 Nagle（凑大段），测延迟必关 Nagle**——混着谈是常见错误

<a id="pnp-05-code"></a>

## C++ 示例：TTCP 核心逻辑

```cpp
// ttcp_client.cpp 核心 — g++ -O2 ttcp_client.cpp -o ttcp
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>
#include <cstdint>

struct SessionMsg {          // 会话头：裸结构体上线的对与错见 09
    int32_t number;          // 块数
    int32_t length;          // 每块字节数
};

ssize_t writen(int fd, const void* buf, size_t n);   // 见 02

double nowSec() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);   // 单调钟，不受 NTP 跳变影响
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    const int    nblocks = 1024, buflen = 65536;
    const int    one = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(9001);
    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));  // 实验变量之一

    SessionMsg msg{ htonl(nblocks), htonl(buflen) };
    writen(fd, &msg, sizeof(msg));

    std::vector<char> block(buflen, 'x');
    double t0 = nowSec();
    for (int i = 0; i < nblocks; ++i)
        if (writen(fd, block.data(), buflen) < 0) { perror("write"); return 1; }
    msg.number = 0;                                  // 结束标记
    writen(fd, &msg, sizeof(msg));

    int32_t ack = 0;
    read(fd, &ack, sizeof(ack));                     // 等服务端确认
    double dt = nowSec() - t0;
    double mb = double(nblocks) * buflen / 1e6;
    printf("sent %.0f MB in %.3f s = %.1f MB/s (%.2f Gbit/s)\n",
           mb, dt, mb / dt, mb * 8 / dt / 1000);
    close(fd);
}
```

服务端对称：`readn` 头部 → 循环 `readn` 每块 → 读到 0 长度标记 → 回写总字节数。

<a id="pnp-05-kernel"></a>

## 内核视角

- `write()` 大块数据 → 拆成 MSS 段进 `sk_write_queue` → 拥塞控制（cwnd）与 Nagle 决定实际发送时机；**发送缓冲满时 write 阻塞（或非阻塞返回 EAGAIN）**——用户态缓冲再大也受 `sk_sndbuf` 闸门
- 批量读：`read(fd, buf, 64K)` 一次 `tcp_recvmsg` 把接收队列上多个 sk_buff 的数据 **合并拷出**——这就是"粘包"在性能上的红利：系统调用摊销
- GSO/GRO：内核把多个段合并处理（发送/接收侧 offload），`ethtool -k eth0` 查看——虚拟机/云主机上这些开关对吞吐影响巨大
- 自动调优：接收窗口由 `tcp_rmem` 按需增长；手工 `setsockopt(SO_RCVBUF)` 一旦设置反而 **关掉** 自动调优（设的是硬上限）

<a id="pnp-05-pitfalls"></a>

## 坑点

- 小包 + Nagle → 吞吐假象
- 用户态缓冲太小导致系统调用过多
- 用 `time()`/`gettimeofday` 测性能（分辨率/跳变问题），应 `CLOCK_MONOTONIC`
- 忘记等对端 ack 就计时结束：测的是"灌满发送缓冲的速度"不是端到端吞吐（上例的 ack 不可省）
- 测回环当真实性能：回环无网卡/NIC offload/中断开销，数字偏乐观一个量级
- 设置 `SO_SNDBUF` 反而关闭了自动调优，吞吐不升反降

<a id="pnp-05-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 风控/清算链路 | 非关键路径走大批量 TCP（成交回报归集），TTCP 方法论直接适用 |
| 测量纪律 | 指标定义（吞吐 vs 延迟 vs P99）、时钟源、预热、多次取样——HFT 基准测试的全部常识从这里起步 |
| 延迟测试 | 小块 + `TCP_NODELAY` + P99/P99.9 分位数统计；进一步见 [15 系统性能](../../06.6-systems-performance/) |
| 硬件上限 | 万兆网卡线速 1.25GB/s，内核 TCP 大约能到 60-80%，再往上就是 [13 DPDK](../../13-dpdk/) 的领地 |

<a id="pnp-05-quiz"></a>

## 自测题

1. 为什么 `write` 返回成功不代表数据已离开主机？接收方什么时候才能"看到"这 1 GiB 的最后一字节？
2. 1 KiB 缓冲与 64 KiB 缓冲传 1 GiB，系统调用差多少次？每微秒 1 次的系统调用开销总共多花多少时间？
3. 跨机房 RTT 30ms、带宽 10Gbps，需要的 TCP 窗口至少多大（BDP）？默认 sndbuf 够吗？
4. 什么情况下设置 `SO_RCVBUF` 会让吞吐下降？
5. TTCP 结束为什么要等 ack 再计时？

<a id="pnp-05-refs"></a>

## 交叉引用

- 上一篇：[04 Netcat](./04_Netcat.md) · 下一篇：[06 非阻塞 I/O](./06_NonBlockingIO.md)
- [02 粘包](./02_TCPByteStream.md)（readn/writen） · [12 TCP/IP 拥塞控制](../../11-tcpip-protocols/) · [15 系统性能](../../06.6-systems-performance/)
