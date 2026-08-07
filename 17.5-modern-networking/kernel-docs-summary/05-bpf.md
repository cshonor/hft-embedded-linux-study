# 05 — Documentation/bpf/

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/bpf/`

## 文档概述

eBPF 官方文档目录，涵盖 BPF 程序类型、verifier、工具链、指令集等。

## 与网络相关的 BPF 程序类型

| 程序类型 | 挂载点 | 用途 |
|---------|--------|------|
| BPF_PROG_TYPE_XDP | 网卡驱动层 | 收包最早点处理 |
| BPF_PROG_TYPE_SCHED_CLS | tc ingress/egress | 流量分类 |
| BPF_PROG_TYPE_SCHED_ACT | tc action | 流量动作 |
| BPF_PROG_TYPE_CGROUP_SKB | cgroup | 进程组网络过滤 |
| BPF_PROG_TYPE_CGROUP_SOCK_ADDR | connect/bind | socket 地址拦截 |
| BPF_PROG_TYPE_SK_REUSEPORT | SO_REUSEPORT | 连接分发 |
| BPF_PROG_TYPE_SK_MSG | socket sendmsg | socket 消息拦截/重定向 |

### BPF Map 类型

| Map 类型 | 网络用途 |
|---------|---------|
| BPF_MAP_TYPE_DEVMAP | XDP redirect 到网卡 |
| BPF_MAP_TYPE_CPUMAP | XDP redirect 到 CPU |
| BPF_MAP_TYPE_XSKMAP | AF_XDP socket 查找 |
| BPF_MAP_TYPE_SOCKMAP | socket 重定向 |
| BPF_MAP_TYPE_LPM_TRIE | 最长前缀匹配（路由） |

### Verifier 限制

- 程序大小有限（100 万指令）
- 不能有无限循环
- 不能解引用空指针
- 必须检查 data_end 边界
- 不能直接访问内核内存（需通过 helper）

## HFT 要点

- XDP + XSKMAP 实现 AF_XDP 路径
- CPUMAP + BPF 实现行情多核分发
- verifier 限制：复杂的包解析逻辑需注意指令数限制
