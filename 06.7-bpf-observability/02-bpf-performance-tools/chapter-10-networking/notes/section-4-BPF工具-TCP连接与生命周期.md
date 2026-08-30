# 4. BPF 工具：TCP 连接与生命周期（10.3.9–10.3.13）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p470–485）

覆盖 5 个工具：soconnlat、solstbyte、tcpconnect、tcpaccept、tcplife。

## 4.1 soconnlat —— TCP 连接建立延迟

```
PID   COMM         LAT(ms)
```

- 逻辑：connect() 返回 **EINPROGRESS**（非阻塞已发出）后，等 **poll/select 成功**才算连接完成——延迟 = 发起→可写。
- 捷径假设：连接完成与发起在同一线程（`ustack` 输出用户栈辅助定位调用方）。

## 4.2 solstbyte —— TCP 首字节延迟（TTFB）

- connect 成功 → 该 fd 上**首次 read** 的时间差。
- 实现：`@connstart[pid, fd]` 为键存起始时间，read 命中即输出并清理，close() 时删除防泄漏。

## 4.3 tcpconnect —— 主动连接（BCC）

```bash
tcpconnect -t          # 含时间戳
tcpconnect -P 1313     # 过滤端口
```

- kprobe `tcp_v4_connect`（入口取 sockaddr、出口取返回值与耗时）。
- **tcpconnect-tp.bt**：跟踪点版用 `sock:inet_sock_set_state` 过滤 `TCP_CLOSE→TCP_SYN_SENT` 转换，无需 kprobe。

## 4.4 tcpaccept —— 被动接受（BCC）

- kprobe `inet_csk_accept`：它是 `tcp_prot.accept` 成员函数——**等所有 accept 到达点**（比跟踪 syscalls:accept 更底层更全）。
- IPv6 输出 `::ffff:x.x.x.x` 映射地址（v4-mapped）。
- **tcpaccept-tp.bt** 状态切换版**无 PID**：状态切换发生在软中断上下文，取 pid 不可靠（10.1.4 错误一）。

## 4.5 tcplife —— 连接全程画像

```
PID   COMM       LADDR           LPORT RADDR           RPORT TX_KB RX_KB MS
```

- 以 sock 指针为键缓存 PID/comm（解决软中断上下文归属问题），close 时输出 TX/RX 字节与存活时长。
- 用途：连接时长分布、谁在建立短连接、断线重连风暴识别。

## HFT 关联

- **下单延迟第一跳 = soconnlat（建连）+ solstbyte（TTFB）**：交易所前置机建连慢→检查 SYN 队列；TTFB 高→对端应用慢。
- tcplife 的 MS 分布可量化"会话保活 vs 短连接"策略收益；断线重连风暴直接看 tcpconnect 速率。

<details>
<summary>自测题</summary>

1. soconnlat 为什么不能在 connect() 返回处就算延迟？
2. tcpaccept 跟踪 inet_csk_accept 而非 accept 系统调用的好处？
3. tcplife 如何在软中断上下文拿到正确的 PID？
4. solstbyte 的 @connstart 键是什么？何时清理？
</details>
