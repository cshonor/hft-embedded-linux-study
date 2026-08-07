# 01 — 网络栈架构

> **Bootlin 课程模块：** Network Stack Architecture
> **对应 Rosen:** Ch1

## 课程内容

### Linux 网络栈全景

```
用户态:  application → socket API
           ↓
内核态:  socket layer → TCP/UDP → IP → routing → tc → driver → NIC
           ↑                                                    ↓
           └──────────── XDP hook (最早点) ←─────────────────────┘
```

### 核心数据结构

| 结构 | 作用 | 现代变化 |
|------|------|---------|
| `struct net_device` | 网卡抽象 | 新增 XDP 相关字段 |
| `struct sk_buff` | 包数据 + 元数据 | 逐步与 xdp_buff 分流 |
| `struct xdp_buff` | XDP 路径包数据 | 轻量，sk_buff 之前 |
| `struct sock` | socket 抽象 | 新增 sk_reuseport / BPF |
| `struct net` | 网络命名空间 | 容器网络隔离 |

### 关键子系统

| 子系统 | 作用 | 对应 Rosen |
|--------|------|-----------|
| socket layer | 用户态接口 | Ch11 |
| 协议栈 | TCP/UDP/IP | Ch4/Ch11 |
| 路由 | FIB 查找 | Ch5/Ch6 |
| Netfilter/nftables | 包过滤 | Ch9 |
| Traffic Control | QoS | Ch6 |
| XDP | 早数据路径 | 无 |
| NAPI | 收包轮询 | Ch1/Ch14 |

## HFT 要点

- 理解完整数据路径是延迟优化的前提
- XDP 是最早 hook 点，之后是 tc ingress，之后是协议栈
- 每经过一层，延迟增加、灵活性增加
