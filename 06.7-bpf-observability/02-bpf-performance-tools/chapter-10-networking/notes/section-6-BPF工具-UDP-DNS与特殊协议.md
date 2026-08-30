# 6. BPF 工具：UDP、DNS 与特殊协议（10.3.20–10.3.23 + 10.3.31）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p500–510）

覆盖：udpconnect、gethostlatency、ipecn、superping，以及 10.3.31 其他工具。

## 6.1 udpconnect —— UDP"连接"事件

- kprobe `ip4_datagram_connect` / `ip6_datagram_connect`（connect() 用于 UDP 时固定对端）。
- **端口大端序翻转**与 TCP 工具相同。
- 用途：发现偷偷绕过 TCP 走 UDP 的服务。

## 6.2 gethostlatency —— DNS 解析延迟

```
TIME   PID    COMM          LATms HOST
```

- **uprobe**：libc `getaddrinfo / gethostbyname / gethostbyname2`（入口存 host，出口算延迟）。uretprobe 返回后输出。
- 案例：**Shopify Kubernetes** 并发 DNS 解析放大 → 触发云厂商 **UDP 连接数限制** → 调大配额解决。
- 注意：仅覆盖 glibc 路径；静态链接/自研 resolver（如 systemd-resolved 直连）测不到。

## 6.3 ipecn —— IP 显式拥塞通知（概念验证）

- 读 IP 头 `tos & 3 == 3`（ECT(1)+CE）标记拥塞通告。
- kprobe `ip_rcv`；**内联函数无法 kprobe**——选函数时先确认非 inline（funccount 验证）。

## 6.4 superping —— 内核态 ICMP 测量

```
PACKET_TYPE  LAT(ms)
```

- 以 `[id, seq]` 为键配对：`ip_send_skb`（发）→ `icmp_rcv`（收），两次内核态打点。
- 对比 `ping -U`（用户态打点）：差值 **0.10ms** = 用户态调度噪声——证明内核态测量更准。

## 6.5 10.3.31 其他相关工具

| 工具 | 来源 | 用途 |
|---|---|---|
| solisten | BCC | 监听套接字变化表 |
| tcpstates | BCC | TCP 状态机迁移时间线（跟踪点 sock:inet_sock_set_state） |
| tcpdrop | BCC | 内核丢弃的段 + 原因（与 tcpretrans 配对） |
| sofdsnoop | BPF Observability | fd 传递（SCM_RIGHTS）到 unix socket |
| profile / hardirq / softirq | ch6 | 排除 CPU/中断瓶颈 |
| filetype | ch8 | 流量落盘特征 |

## HFT 关联

- **行情组播/UDP 行情接收异常**：udpconnect 确认对端绑定、`ss -u -a` 看缓冲；丢包配合 ch8 生物层无关注。
- gethostlatency 覆盖 HFT 最隐蔽的坑：**启动时集中 DNS 解析**（柜台域名）造成首单延迟——预解析/hosts 直写是标准做法。
- superping 的"用户态 vs 内核态打点差"思想适用于一切延迟度量：测量点越靠近内核越准。

<details>
<summary>自测题</summary>

1. gethostlatency 用什么探针？测不到哪些 resolver？
2. superping 用什么键配对请求与响应？与 ping -U 的差值说明什么？
3. ipecn 教训：为什么有些函数 kprobe 不到？
4. tcpdrop 与 tcpretrans 如何配合定位丢包位置？
</details>
