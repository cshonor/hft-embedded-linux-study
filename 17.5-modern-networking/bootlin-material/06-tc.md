# 06 — Traffic Control

> **Bootlin 课程模块：** Traffic Control
> **对应 Rosen:** Ch6

## 现代 tc 架构

```
发送方向:     socket → qdisc → class → filter → driver
              ↑          ↑        ↑        ↑
              tc-BPF    fq/codel  u32    cls_bpf

接收方向:     driver → tc ingress → 协议栈
                         ↑
                     tc-BPF / cls_bpf
```

## 常用 qdisc

| qdisc | 用途 | HFT 适用 |
|-------|------|---------|
| pfifo_fast | 先进先出（默认） | 简单够用 |
| fq | Flow pacing（TCP 发送节奏） | 行情转发流 |
| fq_codel | fq + AQM（延迟控制） | 非交易流 |
| tbf | Token Bucket（带宽限制） | 限制非交易流量 |
| etf | Earliest Tx First（时间感知） | HFT 交易流调度 |

## HFT tc 配置示例

```bash
# 交易流走高优先级队列
tc qdisc add dev eth0 root handle 1: prio
tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32     match ip dport 8001 0xffff flowid 1:1

# 非交易流带宽限制
tc qdisc add dev eth0 parent 1:3 handle 30: tbf     rate 10mbit burst 10kb latency 50ms
```
