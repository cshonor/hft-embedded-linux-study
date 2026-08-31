# Chapter 11: 包过滤与 Flowtable

> 来源：kernel-docs（`Documentation/networking/filter.rst` + `nf_flowtable.rst`）+ **v6.6 源码逐条核对**
> 对标：Rosen（无 flowtable——3.x 仅 Netfilter 慢路径；socket filter 见 Ch1/Ch9）
> 内核版本：以 **v6.6** 为准，机制、常量、行号均取自源码
> （`net/core/filter.c`、`net/netfilter/nf_flow_table_{core,ip,inet}.c`、
> `nft_flow_offload.c`、`nf_tables_api.c`、`include/net/netfilter/nf_flow_table.h`、
> `include/uapi/linux/bpf_common.h`）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [packet-filter](notes/01-packet-filter.md) | **包级过滤器演进**：cBPF 指令集与返回值语义（截断长度非布尔）、v6.6 内核已无 cBPF 解释器（`bpf_migrate_filter` 翻译成 eBPF → verifier → JIT）、`SO_ATTACH_FILTER` vs `SO_ATTACH_BPF`、socket filter 是最晚的内核过滤点（连 `data` 都禁访）、五过滤点成本账单 |
| 2 | [nf-flowtable](notes/02-nf-flowtable.md) | **流级快路径**：两半架构（ingress 拦截 + forward 链建流）、`nf_flow_offload_ip_hook` 逐行解析（MTU/TCP 状态/路由缓存三不变量 + 增量 NAT + TTL）、`NF_STOLEN` 语义、双向 tuple 与 de-NAT、30s 超时 GC、硬件 offload（`flags offload`） |

## 本篇的核心结论

1. **⭐ v6.6 内核里没有 cBPF 解释器。** `SO_ATTACH_FILTER` 进来的 cBPF 被
   `bpf_migrate_filter()`（filter.c:1242）翻译成 eBPF 再 JIT——tcpdump 的过滤器
   跑的是原生机器码，「cBPF 解释执行慢」是过时知识。

2. **⭐ cBPF 返回值是截断长度，不是布尔**：0 = 丢包，k = 保留前 k 字节
   （tcpdump `-s` 的实现机制）。

3. **⭐ socket filter 是最晚的内核过滤点**：跑在 `sock_queue_rcv_skb()` 入口，
   完整协议栈已付出；且 `SOCKET_FILTER` 类型连 `data`/`data_end` 都禁访
   （能力由位置语义决定，详见 [chapter-09/02 §4.2](../chapter-09-tc-bpf/notes/02-tc-bpf.md)）。

4. **⭐ flowtable 拦截点强制 netdev ingress**（`nf_tables_api.c` 校验
   `hooknum == NF_NETDEV_INGRESS`），`flow offload` 表达式强制 FORWARD 链
   （`nft_flow_offload.c:385`）——快路径语义是「绕过整个 IP 栈」。

5. **⭐ 快路径缓存的是「查询」，不缓存「不变量」**：MTU、TCP 状态、路由有效性
   每包都查（O(1)）；NAT 走增量 checksum；TTL 照减。命中包 `NF_STOLEN`
   从设备层直发——IP 层 tracepoint/nft counter 都看不到，排障别误判为丢包。

6. **⭐ flowtable 加速的是转发吞吐，不是本机收包延迟**——HFT 行情机
   （本机终结）用不上；行情前置网关/分流器才是它的主场。

## HFT 关联

- **socket filter 的现代用法只有一种**：`SO_ATTACH_BPF` + map（用户态服务实时更新
  白名单），适合多进程共享 socket 的粗筛；行情早过滤必须在 XDP
- **过滤点位置账单**：XDP（仅 DMA）< tc ingress（+skb）< nft INPUT（+路由/conntrack）
  < socket filter（+完整栈）< 用户态（+唤醒/系统调用）
- **flowtable 顺带受益场景**：行情前置机/风控网关的 L3/L4 分流——forward 流量
  offload 后 CPU 留给行情处理
- **per 包逻辑在 offload 后失效**（nft limit 等）——吞吐换灵活性的取舍与 XDP 同构
- **硬件 offload 是 CPU 零参与的唯一路径**：比一切软件快路径都快，依赖 mlx5/ice 类网卡
- **tcpdump 内核态预过滤**（libpcap→cBPF→翻译→JIT）是低成本观测方案

## 交叉引用

- `12.5-modern-networking/chapter-08-ebpf-cgroup-bpf/`：eBPF 类型系统（SOCKET_FILTER 的能力残缺）
- `12.5-modern-networking/chapter-10-nftables/`：flowtable 的宿主体系（hook/优先级/conntrack）
- `12.5-modern-networking/chapter-05-xdp-architecture/`：XDP（逐包可编程的对照解法）
- `12.5-modern-networking/chapter-07-xdp-redirect-dpdk/`：高性能转发的另一条路
