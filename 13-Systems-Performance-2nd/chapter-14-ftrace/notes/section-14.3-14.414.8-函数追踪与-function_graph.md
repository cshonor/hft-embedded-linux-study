## 14.3–14.4、14.8 函数追踪与 function_graph

### Function Tracing

追踪内核函数 **入口** — 极高体积，必须 **filter**。

```bash
TR=/sys/kernel/tracing
echo function > $TR/current_tracer
echo tcp_v4_rcv > $TR/set_ftrace_filter   # 示例：仅追踪该函数
echo 1 > $TR/tracing_on
# ... 产生负载 ...
echo 0 > $TR/tracing_on
cat $TR/trace | head -50
```

**风险：** 无 filter 的 function tracer **打爆 buffer、严重拖慢系统** — 仅短窗口 + 窄 filter。

### function_graph 追踪器

同时追踪 **入口 + 出口** → **调用图 + 每函数耗时**。

```bash
TR=/sys/kernel/tracing
echo function_graph > $TR/current_tracer
echo tcp_recvmsg > $TR/set_graph_function    # 图追踪起点
echo 1 > $TR/tracing_on
sleep 2
echo 0 > $TR/tracing_on
cat $TR/trace | head -80
```

**输出形态：**

```
tcp_recvmsg() {
  __skb_recv_datagram() {
    ...
  } /* 2.345 us */
} /* 5.678 us */
```

**HFT 用途：**

- 内核栈 **收包路径** 慢 — 从 `tcp_v4_rcv` / `udp_rcv` 往下追（对照 [09 Rosen](../../../11-Linux-Kernel-Networking/)）。
- 对比 **DPDK 旁路** 绕过了哪些函数（→ [12-DPDK](../../../12-DPDK-Low-Latency-Network/)）。

→ Ch 13 [perf 与 tracepoint 关系](../../chapter-13-perf/)

---


---

← [本章导读](../README.md)
