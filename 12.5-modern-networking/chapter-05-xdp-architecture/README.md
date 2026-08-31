# Chapter 05: XDP 架构

> 来源：Bootlin（XDP 概述）+ kernel-docs（XDP rings 设计）+ LWN（XDP 架构深度）
> 对标：Rosen（**无 XDP**，3.x 时代该技术不存在，本章是纯增量）
> 内核版本：以 **v6.6** 为准，常量取自 `include/uapi/linux/if_link.h`、`include/uapi/linux/if_xdp.h`、`include/uapi/linux/bpf.h`

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [xdp-bootlin](notes/01-xdp-bootlin.md) | **实操**：三种模式与 `XDP_FLAGS_*`、三条工具链、6 条加载失败清单、7 个可验证实验 |
| 2 | [xdp-rings](notes/02-xdp-rings.md) | **AF_XDP 的四个 ring**：UMEM 布局、chunk 所有权转移、`smp_wmb/smp_rmb` 必要性（含 ARM vs x86 代价差）、`xdp_statistics` 六计数器诊断表 |
| 3 | [xdp-architecture-lwn](notes/03-xdp-architecture-lwn.md) | **架构全景**：五个动作、generic 模式触发拷贝的三个条件、verifier 能力边界、XDP vs DPDK |

> ⚠️ **第 2 篇的分工说明**：`02-xdp-rings.md` 讲的是 **AF_XDP 的 ring**（UMEM 是用户态内存），
> 属于 chapter-06 的主题域。放在本章是因为它是"XDP 框架"的官方设计文档
> （`Documentation/networking/xdp-rings-design.rst`）。**想看 AF_XDP 的 socket API 与收发代码，
> 直接跳 [chapter-06](../chapter-06-af-xdp/)**。

## 本篇的五个核心结论

1. **`XDP_ABORTED == 0`。** `enum xdp_action` 里 0 是"异常 + 丢包 + 触发 tracepoint"，
   **不是**"放行"。返回 0 的程序会静默丢光所有包，而 `bpftool prog show` 显示它正常运行。
2. **XDP 在 skb 分配之前运行**，这是它与 tc/Netfilter 的根本分野：
   没有 `sk_buff` 就没有协议栈的一切开销，代价是**没有 skb 就没有 skb 上的所有便利**
   （无 GRO、无 socket 关联、无 skb 元数据）。
3. **generic XDP 会先拷贝**。满足 `skb_cloned()` || `skb_is_nonlinear()` ||
   `skb_headroom() < XDP_PACKET_HEADROOM` 任一条件就 `pskb_expand_head()`。
   加载时不指定模式 → **静默降级到 generic**，性能归零但程序照跑。
4. **verifier 的三条硬规则**：有界循环、指针算术必须先验边界检查、不能任意调内核函数
   （只能调 BPF helper）。写不出 stdlib 风格的代码不是工具链问题，是模型约束。
5. **XDP 与 DPDK 的分界是"谁拥有网卡"，不是"谁更快"**。
   要看 ARP/ICMP、要跟内核路由表共存 → XDP；要独占网卡、要自己实现一切 → DPDK。

## HFT 关联

- **丢包位置最靠前**：XDP_DROP 在驱动 NAPI poll 里就完成，不分配 skb、不走协议栈，
  单机抗噪能力比 iptables/nftables 高一个量级
- **XDP_TX 做原路反弹**：组播行情的"反射"、行情扇出前的初筛都可以在这里做，
  延迟只有一次 DMA + 一次 doorbell
- **XDP_REDIRECT 到 AF_XDP**：用户态零拷贝收包的唯一路径，
  是 HFT 里唯一能与 DPDK 同台竞技的内核方案（见 chapter-06）
- **generic XDP 只能验正确性，不能测性能**：veth + generic 模式是 CI 的利器，
  但拿它的数字去说服任何人都是误导
- **加载失败优先查这三样**：驱动是否支持 native（降级到 generic）、
  每队列 ring 大小是否与 XDP 程序声明一致、MTU 是否超过单页
- **观测别用 tcpdump 看 XDP 之后的包**：tcpdump 挂在 `ptype_all`（`dev.c:5394`），
  在 XDP **之后**，XDP 丢的包 tcpdump 看不见——要用 `ethtool -S` 的驱动计数和
  `xdp_statistics` 交叉验证

## 交叉引用

- `12.5-modern-networking/chapter-01-net-stack-architecture/`：XDP 在整栈中的精确位置（H1 钩子）
- `12.5-modern-networking/chapter-03-tx-path-skbbuff/`：XDP_PASS 之后 `xdp_buff` 如何变 `sk_buff`
- `12.5-modern-networking/chapter-04-page-pool/`：XDP_REDIRECT / XDP_TX 依赖 page_pool 的保持映射
- `12.5-modern-networking/chapter-06-af-xdp/`：XDP_REDIRECT 的目的地之一，UMEM 零拷贝落地
- `12.5-modern-networking/chapter-07-xdp-redirect-dpdk/`：redirect 机制本体与 DPDK 对比
- `12.5-modern-networking/chapter-09-tc-bpf/`：tc-BPF 同样是 eBPF，但作用在 skb 上（XDP 之后）
- `13-dpdk/`：绕过内核的另一种选择
