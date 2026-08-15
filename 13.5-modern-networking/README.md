# 13.5-modern-networking

> 定位：**现代Linux内核（5.x / 6.x）网络子系统参考资料**
> 前置：`13-kernel-networking`（Rosen《Linux Kernel Networking》，基于3.x）
> 本目录存放现代内核网络栈资料，弥补 Rosen 书的大量过时实现；
> 学习完本目录材料之后，再回到 `13-kernel-networking` 做源码阅读与实操实验。

## 资料来源

1. **LWN.net 深度专题文章**，修正 Rosen 书的过时收包路径、数据结构、过滤框架
2. **内核官方文档** `Documentation/networking/`，跟随 LTS 内核迭代更新
3. **Bootlin 公开培训讲义**，网络子系统课程 + 动手实验指引

> ⚠️ 与 05.5/06.5 的区别：笨叔《奔跑吧Linux内核》不覆盖网络子系统，
> 因此本目录无 `book-ben-shu-notes/`，改用 `kernel-docs-summary/` 补位。

## 章节索引

| 章 | 主题 | 来源 | 目录 |
|----|------|------|------|
| 01 | 网络协议栈架构 | Bootlin | [chapter-01](chapter-01-net-stack-architecture/) |
| 02 | NAPI 与收包路径 | Bootlin + kernel-docs + LWN | [chapter-02](chapter-02-napi-rx-path/) |
| 03 | 发包路径与 sk_buff | Bootlin + kernel-docs + LWN | [chapter-03](chapter-03-tx-path-skbbuff/) |
| 04 | Page Pool | kernel-docs + LWN | [chapter-04](chapter-04-page-pool/) |
| 05 | XDP 架构 | Bootlin + kernel-docs + LWN | [chapter-05](chapter-05-xdp-architecture/) |
| 06 | AF_XDP | kernel-docs + LWN | [chapter-06](chapter-06-af-xdp/) |
| 07 | XDP Redirect 与 vs DPDK | LWN | [chapter-07](chapter-07-xdp-redirect-dpdk/) |
| 08 | eBPF 与 cgroup BPF | Bootlin + kernel-docs + LWN | [chapter-08](chapter-08-ebpf-cgroup-bpf/) |
| 09 | TC 与 BPF | Bootlin + LWN | [chapter-09](chapter-09-tc-bpf/) |
| 10 | nftables | Bootlin + LWN | [chapter-10](chapter-10-nftables/) |
| 11 | 包过滤与 Flowtable | kernel-docs | [chapter-11](chapter-11-packet-filter-flowtable/) |
| 12 | io_uring 网络收发 | kernel-docs + LWN | [chapter-12](chapter-12-io-uring-net/) |
| 13 | 零拷贝与高性能网络 | kernel-docs + LWN | [chapter-13](chapter-13-zerocopy-highperf/) |
| 14 | TCP/UDP 内部机制 | LWN | [chapter-14](chapter-14-tcp-udp-internals/) |
| 15 | 调试与性能调优 | Bootlin | [chapter-15](chapter-15-debugging-perf-tuning/) |

## 学习流转顺序

1. `13-kernel-networking`：理解内核网络栈需要解决什么问题，**不要照搬3.x代码实现**
2. `13.5-modern-networking`：学习5.x~6.x真正的现代网络栈实现
3. `14-dpdk`：用户态旁路网络，HFT 的主力收发路径

### ⚠️ 关键警告

Rosen 的书基于Linux 3.x（2014年）。**Netfilter/iptables→nftables、无XDP、无eBPF网络、无io_uring、无page_pool、无MSG_ZEROCOPY。禁止直接对照源码查找旧API。本目录全部材料用来补齐时代差异。**

### 现代 vs Rosen 3.x 关键差异

| 领域 | Rosen 书里（3.x） | 现代（5.x/6.x） | 本目录章节 |
|------|-------------------|-----------------|-----------|
| 收包路径 | NAPI 基础版 | NAPI + page_pool + XDP hook | ch02, ch04, ch05 |
| XDP | 不存在 | 4.8+ 引入，内核数据路径旁路 | ch05, ch06, ch07 |
| eBPF 网络 | 不存在 | tc-BPF / XDP-BPF 取代大量 Netfilter | ch08, ch09 |
| 包过滤 | Netfilter/iptables | nftables（4.1+ 全面替代） | ch10, ch11 |
| 异步IO | epoll | io_uring（5.1+） | ch12 |
| 零拷贝 | 无 | MSG_ZEROCOPY + TCP zero-copy recv | ch13 |
| sk_buff | 旧结构 | 持续演进，XDP 路径用 xdp_buff | ch03 |

## 参考索引文件

- [ref-networking-outdated-mapping.md](./ref-networking-outdated-mapping.md) — Rosen 过时章节 → LWN/官方文档详细映射
