# 1. 背景知识（10.1 Networking Background）

> 底本：《BPF之巅》第 10 章 网络，10.1 节（印刷 p411–434）

## 1.1 网络软件栈（图 10-1）

```
应用 (write/read)
  ↓ 系统调用层 (send/recv/sendmsg/recvmsg)
  ↓ VFS
  ↓ 套接字层 (sock → socket_file_ops)
  ↓ TCP / UDP / ICMP
  ↓ IP
  ↓ qdisc (排队规则)
  ↓ 设备驱动
  ↓ NIC (物理网卡)
```

- 每个"层"都有对应跟踪点：**syscall 跟踪点 → 套接字层、tcp 跟踪点 + kprobes → TCP、UDP/IP/ICMP 仅有 kprobes、skb/net/qdisc/xdp 跟踪点 → 驱动与旁路**（表 10-1）。
- **内核绕过**：DPDK（用户态轮询 + UIO/VFIO）绕过整个内核栈 → **BPF 跟踪工具全部失效**；XDP 是 BPF 快速通道（包到达驱动即处理），常用于 DDoS 缓解、软件定义路由（SDR）——XDP 程序本身仍可被 BPF 观测。
- **内部实现三结构**：`sk_buff`（每个包一份描述符，贯穿全栈）、`sock`（套接字状态：缓冲区水位/地址等）、`proto`（协议操作表，如 `tcp_prot.sendmsg` 挂载回调）。

## 1.2 缩放技术（多核扩展）

| 技术 | 层级 | 作用 |
|---|---|---|
| RSS | NIC 硬件 | 多队列把中断分散到多核 |
| RPS | 内核 | 软件版 RSS，按哈希转发报文到指定 CPU |
| RFS | 内核 | 结合 socket 所在 CPU，让数据落在应用所在核（缓存友好） |
| XPS | 发送侧 | 发送队列绑核 |
| SO_REUSEPORT + BPF | 应用 | 多进程同端口，BPF 按哈希导流，避免惊群 |

> Netflix 案例：SO_REUSEPORT + BPF 导流做到**每秒 600 万 SYN** 的无锁接收。

## 1.3 TCP 关键机制

- **SYN 积压队列（图 10-2 双队列）**：半开连接进 SYN 积压队列（`/proc/sys/net/ipv4/tcp_max_syn_backlog`），握手完成进监听积压队列（`listen(2)` 第二参数，上限 `somaxconn`）。溢出表现为 SYN 被丢弃/回 RST —— 工具 `tcpsynbl`（10.3.17）。
- **重传**：RTO 首次超时 ≥200ms 且指数回退；快速重传（3 个重复 ACK）毫秒级；SACK 选择性确认减少盲目重传。跟踪**重传事件**而非每包，开销可降 2 个数量级（10 万包/秒 vs 1000 重传/秒 → 开销约 1%）。
- **发送/接收缓冲区（图 10-3）**：MSS、GSO/TSO（发送侧合并分段卸载）、GRO（接收侧合并）。
- **拥塞控制**：默认 Cubic；可选 Reno/Tahoe/DCTCP（数据中心）/BBR（基于带宽-RTT 模型）。`ss -i` 可见 `bbr`、`pacing_rate`。
- **qdisc**：`tc` 命令族管理；BPF 程序类型 `BPF_PROG_TYPE_SCHED_CLS`（分类器）/ `SCHED_ACT`（动作）。
- **发送微调**：Nagle 算法（小包攒批，`TCP_NODELAY` 关闭）、BQL（字节队列限制）、TSQ（每套接字发送队列上限）、**EDT**（Earliest Departure Time，Linux 4.20 定时转轮模型，fq qdisc 配合）。

## 1.4 七种延迟指标

| 指标 | 含义 | 工具 |
|---|---|---|
| 名字解析延迟 | getaddrinfo 等耗时 | gethostlatency |
| Ping 延迟 | ICMP RTT | ping / superping |
| TCP 连接延迟 | SYN→ACK→ACK 完成 | soconnlat / tcpconnect |
| TCP 首字节（TTFB） | 连接完成→首个数据字节 | solstbyte |
| TCP RTT | 数据往返（内核估算） | tcpwin / ss -i |
| 连接时长 | 建立→关闭全程 | tcplife |
| TFO | TCP Fast Open， SYN 携带数据 | — |

## 1.5 BPF 能力（10.1.2）与十步策略（10.1.3）

- TCP 跟踪点 Linux 4.15/4.16 起加入：`tcp:tcp_retransmit_skb / send_reset / receive_reset / destroy_sock / rcv_space_adjust / retransmit_synack / probe` 等；**裸跟踪点（raw tracepoint）最高效**（无 BTF 参数重排开销）。
- 十步分析策略：① 先看计数器（nstat/sar）→ ② `tcplife` 定性连接 → ③ 确认接口物理上限（ethtool）→ ④ 重传/罕见事件（tcpretrans/tcpdrop）→ ⑤ DNS（gethostlatency）→ ⑥ 多角度延迟（soconnlat/solstbyte/tcpwin）→ ⑦ 负载生成验证 → ⑧ 上 BPF 工具细查 → ⑨ CPU 采样（profile）排除本机瓶颈 → ⑩ 跟踪点/kprobes 钻内核内部。

## 1.6 常见跟踪错误（10.1.4）

1. **事件不在应用上下文触发**：软中断上下文里 `pid/comm` 是 kworker 而非业务进程 → 需以 `sock` 指针为键**缓存** PID/进程名（socketio/tcplife 均如此）。
2. **快/慢路径之分**：如 `tcp_sendmsg` 有 fast path（内存足够直接拷贝）与 slow path（锁竞争/拷贝失败），只跟踪一个函数易漏事件。
3. **满/不满套接字字段无效**：读 `sock` 字段要判断状态（如未连接时 RTT 无意义）。

## HFT 关联

- 行情组播/订单单播延迟拆解正对应**七种延迟指标**：TTFB（行情首包）、连接时长（断线重连风暴）、RTT（交易所往返抖动）。
- HFT 网卡多为 ONLOAD/DMA 大页方案——等价"内核绕过"，工具选择参考 1.1：绕过路径 BPF 失效，需厂商自带计数器。
- `SO_REUSEPORT + BPF` 导流是行情分发进程模型的标准做法（多进程各绑一核）。

<details>
<summary>自测题</summary>

1. DPDK 为什么无法用 BPF 跟踪？XDP 为什么可以？
2. SYN 积压队列与监听积压队列分别由哪个参数控制？哪个 BPF 工具观测？
3. 为什么 tcpretrans 跟踪重传而不是每个数据包？开销差多少？
4. 三条常见跟踪错误中，"事件不在应用上下文"如何用 sock 指针解决？
</details>
