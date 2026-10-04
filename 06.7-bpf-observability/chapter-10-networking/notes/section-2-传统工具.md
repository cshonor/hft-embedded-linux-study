# 2. 传统工具（10.2 Traditional Tools）

> 底本：《BPF之巅》第 10 章 网络，10.2 节（印刷 p434–447）

## 2.1 工具总览

| 工具 | 数据源 | 一句话定位 |
|---|---|---|
| ss | netlink | 套接字全量快照，`-tiepm` 一把梭 |
| ip | netlink | 链路状态与错误分类 |
| nstat | /proc/net/snmp 等 | TCP/IP 计数器，默认**重置**读数 |
| netstat | /proc | 老牌全家桶（连接/接口/协议统计） |
| sar | 多种 | 历史趋势（-n SOCK,TCP,ETCP,DEV） |
| nicstat | /proc/net/dev | %util 饱和度（netstat 没有） |
| ethtool | 驱动 | 驱动统计/信息/特性开关 |
| tcpdump | libpcap | 抓包（内核状态/PID/调用栈盲区） |
| /proc/net | procfs | snmp/netstat/tcp/udp、interrupts |

## 2.2 ss —— 套接字显微镜

```bash
ss -tiepm       # 全量列：内部栈、扩展、进程、内存
```

扩展列要点：`rto`（重传超时）、`rtt`（平滑 RTT 及方差）、`mss`、`cwnd`（拥塞窗口）、`bbr`（算法）、`pacing_rate`（节奏速率）。TCP 状态内幕（-e）：`timer`（重传定时器 on/off/keepalive）、`skmem`（内存分配/压力）。

## 2.3 ip —— 链路与错误分类

```bash
ip -s link      # RX: errors/dropped/overrun；TX: carrier/collsns
ip route        # 路由表，策略路由排查
```

- RX errors = 校验错等硬错误；dropped = 内核缓冲不足；overrun = FIFO 溢出（硬件饱和信号）。
- TX carrier = 物理层/链路协商问题；collsns = 冲突（现代交换网络几乎不该出现）。

## 2.4 nstat —— 计数器与"重置陷阱"

```bash
nstat -S        # -S = 不重置计数器（snapshot）
```

- nstat 默认**读取即重置**计数器——对比"问题前后"必须 `-S` 各拍一次快照再求差。
- 关键计数器：`TcpRetransSegs`、`TcpExtTCPTimeouts`、`TcpExtListenDrops`、`TcpExtTCPSynRetrans`。

## 2.5 sar / netstat / nicstat

```bash
sar -n SOCK,TCP,ETCP,DEV 1
```

- SOCK（套接字总量）、TCP（主动/被动打开、失败尝试）、ETCP（重传、失败段）、DEV（接口速率与队列压力 `rxdrop/txdrop`）。
- nicstat：**%util = 接口饱和度**（netstat 没有的关键指标），`-U` 分读写方向。

## 2.6 ethtool —— 驱动层三件套

```bash
ethtool -S eth0    # 驱动私有统计（队列级收发包/中断）
ethtool -i eth0    # 驱动名与固件版本
ethtool -k eth0    # 特性开关：TSO/GSO/GRO/校验卸载
ethtool -K eth0 tso off   # 运行时调节
```

## 2.7 tcpdump 的盲区（BPF 的价值）

- 抓包开销大（拷贝整个包到用户态 + 过滤在用户态匹配前就已产生成本）。
- **看不到内核状态**（拥塞窗口/队列深度）、**看不到 PID/comm**、**看不到内核调用栈**。
- BPF 工具（sockio/tcpretrans/skbdrop）以事件方式取"包摘要 + 进程 + 栈"，零拷贝、低开销。

## 2.8 /proc 补充

- `/proc/net/snmp`、`/proc/net/netstat`（TcpExt 系列）、`/proc/net/tcp|udp`（十六进制连接表，ss 的底层之一）。
- `/proc/interrupts` + `/proc/softirqs`：NET_RX/NET_TX 中断在核间是否均衡（RSS 调优依据）。

## HFT 关联

- Runbook 顺序建议：`sar -n DEV,ETCP 1` 看面 → `ss -tiepm` 定位连接 → `nstat -S` 前后差分 → 疑难再上 BPF。
- 行情链路 `ip -s link` 的 overrun/ dropped 突增 = 网卡队列不足或中断绑核失衡的前兆。
- tcpdump 仅用于低频取证（生产高流量链路禁用），日常用 tcpretrans/tcpdrop 替代。

<details>
<summary>自测题</summary>

1. nstat 默认行为是什么？如何安全做"前后对比"？
   <details><summary>答案</summary>默认**读取即重置**计数器——读完这一秒的值，下次读从零开始。安全对比：两次都加 `-S`（snapshot 模式不重置），各自存档再求差。忘加 -S 的话，第二次读到的是"距上次读取以来的增量"，语义完全不同。</details>

2. ss -i 输出里 cwnd、pacing_rate 分别反映什么？
   <details><summary>答案</summary>cwnd=发送端拥塞窗口（当前允许的在途数据量，丢包/拥塞事件后收缩）；pacing_rate=发包节奏速率（把本来突发的窗口摊到时间轴上匀速发出，BBR 与 fq qdisc 配合的关键参数）。前者是"能发多少"，后者是"以什么速率发"。</details>

3. nicstat 的 %util 与 netstat 的差异在哪？
   <details><summary>答案</summary>%util 是**接口饱和度**（接口速率占链路带宽的百分比）——netstat 只有收发计数，没有"离满载还有多远"这个概念。容量规划看 %util，计数排查看 netstat。</details>

4. tcpdump 的四个盲区是什么？BPF 如何补齐？
   <details><summary>答案</summary>① 内核状态盲区（cwnd/队列深度看不到）；② 进程盲区（无 PID/comm）；③ 调用栈盲区（无内核栈）；④ 开销盲区（整包拷贝到用户态+用户态过滤，高流量链路开销巨大）。BPF 工具取"包摘要+进程+栈"，内核态聚合零拷贝。</details>
</details>
