# 08 · UDP 与组播

<a id="pnp-08-goal"></a>

## 目标

无连接、丢包、MTU；组播 `IP_ADD_MEMBERSHIP`、TTL。

<a id="pnp-08-unp"></a>

## UNP 对照

- Ch8 UDP、Ch21–22 多播

<a id="pnp-08-concepts"></a>

## 概念详解

### 1. UDP 的真实语义：数据报 + 尽力而为

TCP 给你的承诺（不丢、不乱、不重、流控、拥塞控制）UDP **一条都不给**，换来的是：

- **保留消息边界**：一次 `sendto` = 一个数据报 = 对端一次 `recvfrom`（≤ 一次，见下）
- 无连接：不用握手、不用 accept，发就走
- 无 Nagle、无重传、无重排——**延迟抖动小且行为可预测**

数据报长度陷阱：`recvfrom` 缓冲如果 **小于** 数据报长度，只返回前 N 字节，**剩余部分被丢弃**（不是"下次继续读"）——和 TCP 字节流语义根本不同。

### 2. connect 一个 UDP socket（不是建立连接）

UDP 的 `connect()` 只做一件事：**固定对端地址**。之后：

- `send`/`recv` 直接用（不用每次填地址）
- 内核收到 **来自其他地址** 的数据报直接丢弃（过滤器）
- 对端 ICMP 错误（端口不可达 → `ECONNREFUSED`）能传回本进程——未 connect 的 UDP 收不到异步错误

单播行情/订单的单一对端场景应该 connect：省每次路由查找，还白送过滤和错误回报。

### 3. 丢包发生在哪：三个丢弃点

| 丢弃点 | 触发条件 | 观测手段 |
|--------|----------|----------|
| 网络路径 | 拥塞、CRC 错误 | 抓包两端对比（[12.5 Wireshark](../12.5-wireshark-packet-analysis/)） |
| **接收 socket 缓冲满** | 应用读太慢，`sk_rcvbuf` 溢出——**静默丢弃，无任何报错** | `netstat -su` 的 `receive buffer errors`；`ss -unm` 看 sk_mem |
| 重组队列 | 分片丢一片则整个数据报丢弃 | `netstat -ip` fragmentation 行 |

UDP 丢包没有重传——**可靠性是应用层的事**（序列号 + gap 检测 + 请求重传/切换备源）。

### 4. MTU 与分片

```
以太网 MTU 1500 = IP 头 20 + UDP 头 8 + payload ≤ 1472
```

- payload > 1472 → IP 分片：**任何一片丢失 = 整个数据报丢失**，且分片/重组消耗路径设备资源
- 现代实践：`sendto` 前就限制 payload ≤ 1472（或路径 MTU 发现 + `IP_MTU_DISCOVER` 设 `IP_PMTUDISC_DO` 置 DF 位）
- 云/隧道环境 MTU 常 <1500（VXLAN 1450），HFT 部署要实测：`ping -M do -s 1472 host`

### 5. 组播：一对多分发

单播要 N 份拷贝，组播让网络设备在分叉点复制——交易所行情分发的标准形态（组播地址 224.0.0.0~239.255.255.255，MAC 层映射 `01:00:5e:xx:xx:xx`）。

接收端三件套：

```cpp
// 加入组播组（触发 IGMP 报文，让上游交换机开始向你转发）
struct ip_mreq mreq{};
inet_pton(AF_INET, "239.255.0.1", &mreq.imr_multiaddr);
inet_pton(AF_INET, "0.0.0.0",     &mreq.imr_interface);   // 或具体网卡 IP
setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

// 发送端两个必设项
u_char ttl = 4;                          // 默认 TTL=1：出不了本网段！
setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
u_char loop = 1;                         // 同主机是否回环给自己（调试用）
setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
```

同机多进程收同一组播：`SO_REUSEADDR`（甚至 `IP_ADD_SOURCE_MEMBERSHIP` 做 SSM 源过滤）。

<a id="pnp-08-code"></a>

## C++ 示例：组播行情接收器骨架

```cpp
// mcast_recv.cpp — g++ -O2 mcast_recv.cpp -o mrecv
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));  // 多接收者
    int rcvbuf = 4 * 1024 * 1024;                                  // 行情突发，加大接收缓冲
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(50000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));   // 必须先 bind 组播端口

    ip_mreq mreq{};
    inet_pton(AF_INET, "239.255.0.1", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    char buf[2048];                        // ≥ MTU 1500，防截断静默丢尾部
    for (;;) {
        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, nullptr, nullptr);
        if (n < 0) { if (errno == EINTR) continue; perror("recvfrom"); break; }
        // 行情帧处理：先读帧头里的 sequence number，检测 gap（丢包自愈的起点）
        // uint64_t seq; memcpy(&seq, buf, 8);
        // if (seq != expect) requestRetransmit(expect, seq);
        printf("datagram %zd bytes\n", n);
    }
    return 0;
}
```

发送端：普通 `sendto` 到 `239.255.0.1:50000`，记得设 TTL。

<a id="pnp-08-kernel"></a>

## 内核视角

- 接收路径：`udp_rcv` → 按四元组（或组播组）查 UDP 哈希表（`udp_table`）→ `__udp_enqueue_schedule_skb`：**接收队列满时直接 `kfree_skb` 丢包**，仅计数（`UDP_MIB_RCVBUFERRORS`）——这就是"静默丢包"的现场
- 每数据报一个 sk_buff，`recvfrom` 一次取走整块（保边界）；与 TCP 不同，**没有 prequeue/quickack 那套状态机**，路径更短、行为更可预测——HFT 喜欢它的根本原因
- 组播接收：网卡按 MAC 组播过滤（ imperfect filtering，杂混模式兜底）→ IGMP snooping 让交换机只向成员端口转发。**VM/容器 bridge 默认开 IGMP snooping 但配置不当时整组丢包**——云上行情收不到，先查交换机端口
- `SO_RCVBUF` 翻倍技巧：内核会把你设的值乘 2（记账含 sk_buff 结构开销），真要 4MB 得设 8MB？不——直接设，然后 `ss -unm` 看实际生效值（`sk_meminfo`）

<a id="pnp-08-pitfalls"></a>

## 坑点

- UDP「发出去」不等于「对端收到」
- 组播在交换机 / Wi‑Fi 上的限制
- 组播 TTL 默认 1：跨网段收不到，以为是代码 bug
- `recvfrom` 缓冲小于数据报 → 静默截断（连 errno 都没有）
- 忘 `bind` 组播端口直接 ADD_MEMBERSHIP → 什么也收不到
- 接收缓冲太小 + 行情突发 → 交换机统计正常但应用层缺号——**丢在内核队列里**
- 同机调试：`IP_MULTICAST_LOOP` 关了导致本机自测收不到

<a id="pnp-08-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| **行情分发** | 交易所主流量行情是 UDP 组播（如 NASDAQ ITCH 5.0 over 组播）；A/B 双馈消除单点丢包——同一份数据两个组播组，取先到者，缺号互相补 |
| 序列号 + gap | 行情帧带 sequence number，接收端检测缺号 → 请求重传通道（通常 TCP）——UDP 可靠性的应用层实现 |
| 内核丢包恐惧 | 行情峰值速率高时内核缓冲丢包是主要事故源 → 加大 RCVBUF、绑核、中断亲和对齐，再往下 [14 DPDK](../14-dpdk/) 用户态直接收组播 |
| 硬件时间戳 | `SO_TIMESTAMPING` 取网卡级时间戳，收包延迟测量（进阶见 [13.5 现代网络](../13.5-modern-networking/)） |

<a id="pnp-08-quiz"></a>

## 自测题

1. `recvfrom` 用 1000 字节缓冲收 1500 字节数据报，返回值是多少？剩下的 500 字节呢？
2. UDP `connect` 后哪些行为变了？为什么单对端场景应该 connect？
3. 行情接收程序缺号（seq 跳跃），列出三个可能的丢弃点及各自的观测命令。
4. 组播 TTL 不设会发生什么？`IP_MULTICAST_LOOP` 什么时候该开？
5. 为什么 HFT 行情用 UDP 而订单常用 TCP？（从延迟确定性与可靠性职责两个角度）

<a id="pnp-08-refs"></a>

## 交叉引用

- 上一篇：[07 epoll](./07_IO_epoll.md) · 下一篇：[09 序列化](./09_Serialization.md)
- [12 TCP/IP 协议（UDP 章）](../12-tcpip-protocols/) · [12.5 Wireshark](../12.5-wireshark-packet-analysis/) · [14 DPDK](../14-dpdk/) · [17 HFT 工程](../17-hft-engineering/)
