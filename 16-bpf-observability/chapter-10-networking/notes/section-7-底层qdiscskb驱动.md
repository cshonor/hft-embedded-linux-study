# 7. 底层：qdisc / skb / 驱动

### `qdisc-*` 家族

针对 **fq、codel、cbq** 等排队规则测量 **包排队延迟**。

**场景：** 出口 bufferbloat、云主机 qdisc 配置不当。

### `netsize`

**GSO/GRO 前后** 设备层 send/recv **包大小直方图**。

### `nettxlat`

**网卡驱动 TX 队列** 延迟 — 包进 ring → 硬件发完。

**HFT：** 区分 **软件栈慢** vs **NIC 发送队列拥塞**（与 `ethtool -S` 配合）。

### `skbdrop`

`sk_buff` **异常丢弃** + **内核栈** — 丢包元凶。

```bash
sudo skbdrop-bpfcc
```

**极 valuable：** `ip -s` 见 drop 但不知原因 → `skbdrop` 给 **函数栈**。

### `skblife`

`sk_buff` 从分配到释放的 **生命周期耗时** — 包在栈里「呆太久」。

### `ieee80211scan`

WiFi 802.11 扫描耗时 — 数据中心 HFT 少见，笔记本调试可用。


### 常见陷阱

1. **忽视 qdisc 排队延迟** — qdisc（队列规则）是内核发送队列，包在 qdisc 中排队等待发送；排队延迟是 HFT 网络抖动的一个来源
2. **混淆 qdisc 丢包和网卡丢包** — qdisc 丢包发生在内核（可 BPF 追踪），网卡丢包发生在硬件（只能 ethtool 统计）；两者原因和检测方式不同
3. **在 HFT 服务器上用复杂 qdisc 规则** — 复杂 qdisc（如 HTB/RED）增加每包处理开销；HFT 应用最简单的 pfifo_fast 或 noqueue，减少排队延迟

<details>
<summary>📝 自测题（点击展开）</summary>

1. **qdisc（队列规则）是什么？对 HFT 延迟有什么影响？**

   <details>
   <summary>参考答案</summary>

   qdisc 是内核网络发送队列的管理规则——包从 TCP 层进入 qdisc 排队，然后由网卡驱动取出发送。排队延迟 = 包在 qdisc 中等待的时间。HFT 影响：(1) 队列长时尾部延迟增加；(2) 复杂 qdisc 规则（HTB/RED）增加 per-packet 处理开销；(3) 队列满时丢包。优化：用最简单的 pfifo_fast 或 noqueue，减少队列深度。

   </details>

2. **如何用 BPF 追踪 qdisc 延迟？**

   <details>
   <summary>参考答案</summary>

   (1) 包入队时间戳：`kprobe:dev_queue_xmit { @qdisc_start[tid]=nsecs }`；(2) 包出队时间戳：`kprobe:dev_hard_start_xmit /@qdisc_start[tid]/ { @qdisc_lat=hist(nsecs-@qdisc_start[tid]); delete(@qdisc_start[tid]) }`；(3) qdisc 丢包：`kprobe:qdisc_drop { @drop++ }`。直方图显示排队延迟分布，尾部异常说明 qdisc 有拥塞。

   </details>

3. **HFT 网络路径上推荐的 qdisc 配置是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 用 `noqueue`（如果只有单队列网卡）或 `pfifo_fast`（最简单的 FIFO）；(2) 避免使用 HTB/TBF/RED 等复杂规则——每包额外计算开销；(3) 减小 txqueuelen：`ip link set eth0 txqueuelen 100`（默认 1000，减少最大排队深度）；(4) 启用网卡 BQL（Byte Queue Limits）自动调节发送队列长度；(5) 对于 HFT 关键路径，考虑用 DPDK/XDP 绕过 qdisc。

   </details>

</details>

---
