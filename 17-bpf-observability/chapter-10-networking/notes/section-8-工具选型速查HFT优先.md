# 8. 工具选型速查（HFT 优先）

| 症状 | 优先工具 |
|------|----------|
| 延迟尖刺、怀疑网络 | **`tcpretrans`** |
| 谁在用带宽 | `tcptop`、`socketio` |
| 意外 outbound 连接 | `tcpconnect`、`soconnect` |
| 连接/会话行为 | **`tcplife`** |
| 连接建立慢 | `soconnlat`、`tcpconnect` |
| 首包慢 | `so1stbyte` |
| DNS 拖慢 | **`gethostlatency`** |
| SYN 丢/满 | `tcpsynbl` |
| 接收缓冲溢出 | `sormem` |
| 内核 drop 不知因 | **`skbdrop`** |
| NIC TX 排队 | `nettxlat`、`ethtool -S` |
| 抓包替代（低开销） | `tcplife` + `tcpretrans` |


### 常见陷阱

1. **选工具时不考虑 HFT 优先级** — HFT 网络排障应优先用低开销工具（tcpretrans/tcpconnlat）而非高开销工具（全系统 send/recv 追踪）
2. **忽视工具的运行时长控制** — 网络 BPF 工具在高 PPS 环境下数据量爆炸；应设置运行时长或用 Map 聚合控制输出量
3. **试图同时运行多个网络 BPF 工具** — 多个 BPF 程序同时 attach 到同一 probe 会叠加开销；应串行排查，一次只运行一个工具

<details>
<summary>📝 自测题（点击展开）</summary>

1. **HFT 网络排障的工具选型优先级是什么？**

   <details>
   <summary>参考答案</summary>

   Tier 1（低开销，长期可挂）：ethtool（网卡统计）+ ss -s（连接概览）+ netstat -s（协议计数）。Tier 2（中开销，分钟级短跑）：tcpretrans（重传追踪）+ tcpconnlat（连接延迟）+ tcprtt（RTT 分布）。Tier 3（高开销，秒级短跑）：socketsnoop（逐包追踪）+ bpftrace tcp_sendmsg 路径分析。从 Tier 1 到 Tier 3 逐步钻取。

   </details>

2. **如何控制网络 BPF 工具的输出量？**

   <details>
   <summary>参考答案</summary>

   (1) 按进程过滤：`-p $(pidof myapp)`；(2) 按端口过滤：`/args->dport == 8080/`；(3) 用 Map 聚合：`@[src,dst] = count()` 替代逐行打印；(4) 设运行时长：`timeout 10 tcpretrans`；(5) 只看异常：`/args->retrans > 0/`；(6) 直方图替代逐事件：`hist()` 看分布而非逐条。

   </details>

3. **为什么不应同时运行多个网络 BPF 工具？**

   <details>
   <summary>参考答案</summary>

   (1) 多个 BPF 程序 attach 到同一 probe（如 tcp_sendmsg），每次命中执行多段 BPF 代码，开销叠加；(2) 多个工具竞争 ring buffer 和 Map 内存；(3) 输出交织难以关联。正确做法：串行排查——先运行 Tier 1 工具收集概览，分析后运行 Tier 2 针对性追踪，一次一个工具。

   </details>

</details>

---
