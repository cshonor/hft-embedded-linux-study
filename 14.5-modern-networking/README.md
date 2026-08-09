# 14.5-modern-networking

> 定位：**现代Linux内核（5.x / 6.x）网络子系统参考资料**
> 前置：`14-kernel-networking`（Rosen《Linux Kernel Networking》，基于3.x）
> 本目录存放现代内核网络栈资料，弥补 Rosen 书的大量过时实现；
> 学习完本目录材料之后，再回到 `14-kernel-networking` 做源码阅读与实操实验。

## 资料来源

1. **LWN.net 深度专题文章**，修正 Rosen 书的过时收包路径、数据结构、过滤框架
2. **内核官方文档** `Documentation/networking/`，跟随 LTS 内核迭代更新
3. **Bootlin 公开培训讲义**，网络子系统课程 + 动手实验指引

> ⚠️ 与 08.5/09.5 的区别：笨叔《奔跑吧Linux内核》不覆盖网络子系统，
> 因此本目录无 `book-ben-shu-notes/`，改用 `kernel-docs-summary/` 补位。

## 内部子目录

- `lwn-articles-summary/`  LWN文章摘要，按主题域分类（收包路径/XDP/eBPF/NAPI/nftables/io_uring/TCP零拷贝）
- `kernel-docs-summary/`  内核 Documentation/networking/ 精选文档摘要
- `bootlin-material/`  Bootlin 网络子系统训练讲义要点 + 实验操作清单

## 学习流转顺序

1. `14-kernel-networking`：理解内核网络栈需要解决什么问题，**不要照搬3.x代码实现**
2. `14.5-modern-networking`：学习5.x~6.x真正的现代网络栈实现
3. `15-dpdk`：用户态旁路网络，HFT 的主力收发路径

### ⚠️ 关键警告

Rosen 的书基于Linux 3.x（2014年）。**Netfilter/iptables→nftables、无XDP、无eBPF网络、无io_uring、无page_pool、无MSG_ZEROCOPY。禁止直接对照源码查找旧API。本目录全部材料用来补齐时代差异。**

### 现代 vs Rosen 3.x 关键差异

| 领域 | Rosen 书里（3.x） | 现代（5.x/6.x） |
|------|-------------------|-----------------|
| 收包路径 | NAPI 基础版 | NAPI + page_pool + XDP hook |
| XDP | 不存在 | 4.8+ 引入，内核数据路径旁路，AF_XDP 零拷贝到用户态 |
| eBPF 网络 | 不存在 | tc-BPF / XDP-BPF 取代大量 Netfilter 功能 |
| 包过滤 | Netfilter/iptables | nftables（4.1+ 全面替代） |
| 异步IO | epoll | io_uring（5.1+），网络收发新路径 |
| 零拷贝 | 无 | MSG_ZEROCOPY 发送 + TCP zero-copy 接收 |
| sk_buff | 旧结构 | 持续演进，XDP 路径用 xdp_buff 绕过 sk_buff |

## 参考索引文件

- [ref-networking-outdated-mapping.md](./ref-networking-outdated-mapping.md) — Rosen 过时章节 → LWN/官方文档详细映射
