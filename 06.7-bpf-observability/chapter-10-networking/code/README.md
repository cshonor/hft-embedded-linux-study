# 02-ch10 code · 网络域 bpftrace 程序

> 《BPF Performance Tools》第 10 章示例落盘。
> 网络纵深在 [12.5-modern-networking](../../../12.5-modern-networking/)，
> XDP/DPDK 对照在 [13-dpdk/02-Advanced-Book](../../../13-dpdk/02-Advanced-Book/)。

## 程序

| 文件 | 出处 | 内容 | 验证（bpftrace 0.20.2） |
|------|------|------|------|
| [sock-rxtx.bt](./sock-rxtx.bt) | §10 块1/2 | socket 收发按 [pid,comm] 拆分 + stats() 字节画像（stats vs hist 选型） | 语义解析 ✓（实跑需 root） |
| [tcp-state-trace.bt](./tcp-state-trace.bt) | §10 块3 | inet_sock_set_state 状态迁移（SYN_SENT=发起连接）+ 软中断上下文 pid 不可信陷阱 | 语义解析 ✓ |

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [net-oneliners.sh](./scripts/net-oneliners.sh) | §2/4/9/11 | 传统工具（ss -tiepm/nstat/ethtool）+ BCC 成品 + bpftrace 六条（connect 失败栈/收发直方图/全链路栈） | `bash -n` ✓ |

## 本章核心：kprobe 精确但脆弱、tracepoint 稳定但语义粗

- `tcp_v4_connect`（kprobe）：有进程上下文（pid 可信），但内核版本间函数可能改名/内联
- `inet_sock_set_state`（tracepoint）：稳定 ABI，但在**软中断上下文**——pid/comm 是被中断的倒霉进程
- 解法：两处以 `args->skaddr`（sock 指针）为键互相补记
