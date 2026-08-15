# 9. BPF 单行程序（10.4 One-Liners）

> 底本：《BPF之巅》第 10 章 网络，10.4 节（印刷 p525–529）

## 9.1 连接失败分析

```bash
# connect 失败按错误码分布
bpftrace -e 't:syscalls:sys_enter_connect /retval < 0/ { @[probe] = count(); }'

# connect 失败 + 用户栈（定位哪段代码在连）
bpftrace -e 'kretprobe:tcp_v4_connect /retval < 0/ { @[ustack(5)] = count(); }'
```

## 9.2 收发字节与频率

```bash
# TCP 发送字节数直方图
bpftrace -e 'kr:tcp_sendmsg { @[comm] = hist(retval); }'

# UDP sendmsg/recvmsg 频率按进程
bpftrace -e 'k:udp_sendmsg, k:udp_recvmsg { @[comm, probe] = count(); }'
```

## 9.3 完整发送路径调用栈

```bash
# net_dev_xmit 的内核栈：write→VFS→socket→TCP→IP→设备 全链路
bpftrace -e 't:net:net_dev_xmit { kstack(15); exit(); }'
```

一次输出即可看到从 write() 系统调用到驱动的完整路径——理解图 10-1 的实证版。

## 9.4 中断驱动接收路径

```bash
# 收包在硬中断/软中断上下文的处理路径
bpftrace -e 't:net:netif_receive_skb { @[kstack(10)] = count(); }'
```

## 9.5 驱动级统计

```bash
# ixgbevf 驱动事件按函数统计
bpftrace -e 't:ixgbevf:ixgbevf_* { @[probe] = count(); }'

# iwlwifi 无线驱动
bpftrace -e 't:iwlwifi:iwlwifi_* { @[probe] = count(); }'
```

## HFT 关联

- 9.3/9.4 的全路径 kstack 是排查"收包路径偶发抖动"的起点：对比正常/异常时刻栈差异，找热点函数（如 `ip_rcv`→`tcp_v4_rcv`→`tcp_recvmsg` 中间插入的 netfilter hook）。
- 驱动级跟踪点（厂商模块自带 tracepoint）比 kprobe 稳定，优先使用。

<details>
<summary>自测题</summary>

1. 如何用一条 bpftrace 看到完整发送路径？
2. 为什么驱动自带 tracepoint 优于 kprobe 驱动函数？
</details>
