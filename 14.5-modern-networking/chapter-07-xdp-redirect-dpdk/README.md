# Chapter 07: XDP Redirect 与 XDP vs DPDK

> 来源：LWN（XDP redirect + XDP vs DPDK 对比）
> 对标：Rosen（无 XDP/DPDK）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [xdp-redirect](notes/01-xdp-redirect.md) | LWN：XDP redirect 机制、bpf_redirect_map、CPUMAP |
| 2 | [xdp-vs-dpdk](notes/02-xdp-vs-dpdk.md) | LWN：XDP vs DPDK 架构对比、性能/生态/灵活性权衡 |

## HFT 关联

- **XDP redirect 到 CPUMAP**：将收包重定向到特定 CPU 的队列，实现 RSS 替代方案
- **XDP redirect 到 AF_XDP**：零拷贝到用户态，HFT 收包路径
- **XDP vs DPDK 权衡**：
  - DPDK：完全 bypass 内核，延迟最低（~100ns），但独占 NIC、需 hugepage、驱动支持有限
  - XDP/AF_XDP：内核态，延迟稍高（~200-500ns），但 NIC 共享、无需 hugepage、驱动支持广
- **HFT 选择**：核心交易路径用 DPDK（极致延迟），监控/备份路径用 AF_XDP（灵活性）

## 交叉引用

- `14.5-modern-networking/chapter-05-xdp-architecture/`：XDP 基础架构
- `14.5-modern-networking/chapter-06-af-xdp/`：AF_XDP redirect 目标
- `15-dpdk/`：DPDK 详细对比
