# Chapter 01: 网络协议栈架构

> 来源：Bootlin（网络栈总览）
> 对标：Rosen Ch1（3.x 架构 → 6.x 架构）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [net-stack-architecture](notes/01-net-stack-architecture.md) | Bootlin：Linux 网络栈分层、socket 层、协议栈、设备驱动层 |

## HFT 关联

- **内核栈 bypass**：HFT 主力路径用 DPDK/AF_XDP 绕过内核协议栈，但理解内核栈架构是设计 bypass 的前提
- **分层开销**：内核网络栈每层都有 skb 克隆/元数据操作，累计延迟 > 1μs；XDP 在协议栈之前拦截
- **socket 层**：`sendmsg`/`recvmsg` 系统调用开销约 100-200ns，io_uring 批量提交可均摊

## 交叉引用

- `13-kernel-networking/`：Rosen 书的 3.x 架构（已过时）
- `14-dpdk/`：DPDK 用户态网络，完全绕过内核
