# Chapter 05: XDP 架构

> 来源：Bootlin（XDP 概述）+ kernel-docs（XDP rings）+ LWN（XDP 架构深度）
> 对标：Rosen（无 XDP，3.x 不存在）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [xdp-bootlin](notes/01-xdp-bootlin.md) | Bootlin：XDP hook 位置、XDP 动作、driver/native/offload |
| 2 | [xdp-rings](notes/02-xdp-rings.md) | kernel-docs：XDP Rx/Tx ring、fill ring、completion ring |
| 3 | [xdp-architecture-lwn](notes/03-xdp-architecture-lwn.md) | LWN：XDP 设计哲学、DMA 直接收包、绕过 sk_buff |

## HFT 关联

- **XDP 位置**：XDP 在 NIC DMA 完成后、sk_buff 分配之前拦截，延迟 < 100ns
- **XDP_DROP**：恶意/无关包在协议栈之前丢弃，不消耗 skb 内存和 CPU
- **XDP_TX**：包直接从 NIC 反射出去，不经过协议栈转发路径
- **XDP_REDIRECT**：包重定向到另一个 NIC 或 AF_XDP socket，实现零拷贝到用户态
- **native vs offload**：native XDP 在驱动层运行（通用），offload XDP 在 SmartNIC 硬件运行

## 交叉引用

- `12.5-modern-networking/chapter-06-af-xdp/`：AF_XDP 将包重定向到用户态
- `12.5-modern-networking/chapter-07-xdp-redirect-dpdk/`：XDP redirect 机制与 DPDK 对比
- `13-dpdk/`：DPDK 完全 bypass 内核，XDP 是内核态替代方案
