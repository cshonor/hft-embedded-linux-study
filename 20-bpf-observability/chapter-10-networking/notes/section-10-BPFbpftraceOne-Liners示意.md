# 10. BPF / bpftrace One-Liners（示意）

```bash
# TCP 重传（生产用 tcpretrans-bpfcc）
# bpftrace -e 'kprobe:tcp_retransmit_skb { printf(...); }'

# 按 comm 统计 connect
bpftrace -e 'tracepoint:syscalls:sys_enter_connect { @[comm] = count(); }'

# 采样内核网络栈（短跑）
bpftrace -e 'kprobe:tcp_sendmsg { @[kstack] = count(); }'
```

→ [Ch 5 bpftrace](../../chapter-05-bpftrace/) · [附录 A](../../appendix-A-bpftrace单行命令.md)


### 常见陷阱

1. **复制 one-liner 不修改网卡名和端口** — one-liner 中的 eth0、端口 80 是示例值，需替换为实际值；不修改可能匹配不到任何流量
2. **忽视网络 one-liner 的 PPS 开销** — 网络 one-liner 在每包触发，高 PPS 环境下开销大；应加 filter 或用 Map 聚合而非逐包 printf
3. **只追踪发送不追踪接收（或反之）** — 网络延迟是双向的——send 延迟 + 网络 RTT + recv 延迟；只看一个方向无法定位问题在哪一段

<details>
<summary>📝 自测题（点击展开）</summary>

1. **网络分析最常用的 3 个 bpftrace one-liner 是什么？**

   <details>
   <summary>参考答案</summary>

   (1) TCP 重传统计：`tracepoint:tcp:tcp_retransmit_skb { @[ntop(args->saddr), ntop(args->daddr)] = count() }`；(2) 连接延迟：`kprobe:tcp_v4_connect { @s[tid]=nsecs } kretprobe:tcp_v4_connect /@s[tid]/ { @connlat=hist(nsecs-@s[tid]) }`；(3) 发送字节数按进程：`tracepoint:syscalls:sys_enter_sendto /comm == "myapp"/ { @[comm] = sum(args->len) }`。

   </details>

2. **如何用 bpftrace 测量网络各层延迟？**

   <details>
   <summary>参考答案</summary>

   (1) 应用→内核：`kprobe:tcp_sendmsg { @app[tid]=nsecs }` 到 `kprobe:ip_queue_xmit /@app[tid]/ { @kern_lat=hist(nsecs-@app[tid]) }`；(2) 内核→网卡：`kprobe:dev_queue_xmit { @qdisc[tid]=nsecs }` 到 `kprobe:dev_hard_start_xmit /@qdisc[tid]/ { @drv_lat=hist(nsecs-@qdisc[tid]) }`；(3) RTT：`tracepoint:tcp:tcp_probe { @rtt = hist(args->rtt / 1000) }`。分段测量定位延迟在哪一层。

   </details>

3. **HFT 网络延迟排查的 one-liner 策略是什么？**

   <details>
   <summary>参考答案</summary>

   三步：(1) 基线测量——tcprtt 直方图看正常 RTT 分布；(2) 异常定位——tcpretrans 看是否有重传（每次重传 = 至少 1 RTT 额外延迟）；(3) 精确分段——bpftrace 在 tcp_sendmsg/dev_queue_xmit/dev_hard_start_xmit 上打时间戳，算出应用→内核→网卡各段延迟，定位抖动来源。加 `/comm == "myapp"/` 过滤目标进程。

   </details>

</details>

---
