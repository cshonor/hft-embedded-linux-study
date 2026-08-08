# P7 — DPDK 转发 + 延迟剖析

> 用 DPDK 写一个 packet forwarder，再用 perf 火焰图和 bpftrace 延迟探针把它剖析透——HFT 收发路径的"性能层"。

## 项目目标

从内核栈跳到用户态旁路。实现一个最小 DPDK 转发器，测量单跳延迟，用性能工具找瓶颈。这是进入 `21` HFT 引擎前的最后一道网络性能关。

## 交付物

- [ ] DPDK 环境：大页、UIO/VFIO、lcore 绑核
- [ ] mempool + mbuf 预分配
- [ ] PMD 轮询收发（rx/tx burst）
- [ ] 最小转发逻辑（MAC 改写 + 转发）
- [ ] 延迟测量：打时间戳，统计 p50/p99/p999
- [ ] perf 采样 + 火焰图，定位热点函数
- [ ] bpftrace 探针，追踪软中断/调度对延迟的扰动
- [ ] 对比：DPDK 旁路 vs 内核栈 (`P6` 抓包路径) 的延迟分布

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`18` dpdk](../../18-dpdk/) | EAL、大页、NUMA、mbuf/mempool、PMD、零拷贝 |
| [`19` systems-performance](../../19-systems-performance/) | perf 采样、火焰图、USE 方法、延迟分解 |
| [`20` bpf-observability](../../20-bpf-observability/) | bpftrace 延迟探针、off-CPU、调度追踪 |

## 前置

[P6](../P6-network-protocol-analyzer/)（内核网络栈与抓包过关）。

## 学习目标

- DPDK 旁路内核栈的原理（UIO/VFIO、PMD 轮询、无系统调用收发）
- 大页 + NUMA 绑定对延迟的影响
- 绑核隔离（isolcpus、`SCHED_FIFO`）在用户态轮询的应用
- perf 火焰图找热点、bpftrace 找抖动源
- 内核栈 vs DPDK 旁路的延迟量化对比

## 里程碑

1. **M1** DPDK 环境跑通 testpmd
2. **M2** 自写 forwarder，能收发
3. **M3** 延迟测量 + 分布统计
4. **M4** perf 火焰图定位热点，优化一轮
5. **M5** bpftrace 找尾延迟源，对比内核栈

## 参考模块

- [18-dpdk/](../../18-dpdk/) — 深入浅出 DPDK、Linux 高性能网络、组播最小工程
- [19-systems-performance/](../../19-systems-performance/) — Gregg 性能之巅 Ch6/7/13
- [20-bpf-observability/](../../20-bpf-observability/) — Gregg BPF 之巅 Ch6/10
