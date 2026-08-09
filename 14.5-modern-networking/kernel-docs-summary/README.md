# 内核文档摘要 — Documentation/networking/ 精选

> 补位笨叔笔记（笨叔不覆盖网络子系统），从内核源码文档中提取现代网络栈关键信息

## 精选文档清单

| 序号 | 文档路径 | 主题 | 对应 Rosen | HFT 关联 |
|------|---------|------|-----------|---------|
| 01 | `Documentation/networking/napi.rst` | NAPI 现代实现 | Ch1/Ch14 | 收包延迟 |
| 02 | `Documentation/networking/page_pool.rst` | page_pool API | 无 | Rx buffer 分配 |
| 03 | `Documentation/networking/xdp-rings-design.rst` | XDP 环形缓冲区设计 | 无 | AF_XDP 零拷贝 |
| 04 | `Documentation/networking/af_xdp.rst` | AF_XDP socket | 无 | 内核态旁路 |
| 05 | `Documentation/bpf/` | eBPF 程序类型与挂载点 | 无 | tc-BPF / XDP-BPF |
| 06 | `Documentation/networking/filter.rst` | socket filter / BPF | Ch9 | 包过滤 |
| 07 | `Documentation/networking/nf_flowtable.rst` | nftables flow table | Ch9 | 流表加速 |
| 08 | `Documentation/networking/txrx.rst` | 驱动收发路径 | Ch1 | NIC→内核数据流 |
| 09 | `Documentation/networking/sock-sk-buff.rst` | sk_buff 生命周期 | Ch11 | sk_buff 分配/释放 |
| 10 | `Documentation/networking/scaling.rst` | RPS/RFS/XPS 多核扩展 | Ch14 | 多核收包分发 |
| 11 | `Documentation/networking/msg_zerocopy.rst` | 零拷贝发送 | 无 | 减少拷贝开销 |
| 12 | `Documentation/networking/io_uring.rst` | io_uring 网络 | 无 | 异步网络 IO |

## 阅读策略

1. 先读 `napi.rst` + `page_pool.rst` 理解现代收包路径
2. 再读 `af_xdp.rst` + `xdp-rings-design.rst` 理解 XDP 旁路
3. 最后读 `scaling.rst` + `msg_zerocopy.rst` 理解性能优化
