#!/usr/bin/env python3
"""Generate 17.5-modern-networking note files: 20 LWN + 12 kernel-docs + 9 bootlin = 41 files."""

import os

BASE = r"C:\Users\12392\Desktop\hft\17.5-modern-networking"

def write_file(relpath, content):
    full = os.path.join(BASE, relpath)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  wrote {relpath}")

# ============================================================
# Part 1: LWN Articles (20 files)
# ============================================================

LWN = [
# --- 1. 收包路径重构 (4) ---
("lwn-articles-summary/01-napi-modern.md", """# 01 — NAPI 现代化：threaded NAPI 与 busy polling

> **对应 Rosen:** Ch1（NAPI 基础）+ Ch14（高级主题 RPS/RFS）
> **内核版本:** NAPI 原始设计 2.5+；threaded NAPI 5.11+；SO_BUSY_POLL 3.11+

## NAPI 基础回顾

NAPI（New API）是 Linux 网卡收包的核心机制：
- 中断驱动 → 轮询模式切换：首个包触发中断后关闭中断，进入轮询
- `struct napi_struct`：每个网卡注册一个 NAPI 实例
- `napi_poll()` 回调：驱动提供的轮询函数，每次调用处理 budget 个包
- 轮询完成后重新开中断，等待下一批

## 现代变化

### Threaded NAPI（5.11+）

传统 NAPI 在软中断上下文（`NET_RX_SOFTIRQ`）执行：
- 软中断与其它 ksoftirqd 共享 CPU，可能被抢占
- 无法绑定到特定 CPU 核心

Threaded NAPI 将轮询移到独立内核线程：
```
# 启用 threaded NAPI
echo 1 > /sys/class/net/eth0/threaded
```
- 每个 NAPI 实例一个内核线程（`napi/eth0`）
- 可通过 `chrt` / `taskset` 设置优先级和 CPU 亲和性
- 代价：线程切换开销，但隔离性更好

### Busy Polling（SO_BUSY_POLL，3.11+）

传统流程：数据到达 → 中断 → 软中断 → socket 可读 → 唤醒用户进程

Busy polling 让用户进程主动轮询 NAPI，跳过中断：
```c
int val = 50;  // busy poll 时间（微秒）
setsockopt(sockfd, SOL_SOCKET, SO_BUSY_POLL, &val, sizeof(val));
```
- `recvmsg()` 时直接调用 `napi_poll()` 检查是否有数据
- 以 CPU 100% 换取低延迟（不等待中断）
- HFT 场景常用：交易进程独占一个 CPU 核心，持续 busy poll

### NAPI budget 变化

- 默认 budget = 64（每次轮询最多处理 64 个包）
- 现代驱动可调整：`ethtool -C eth0 rx-usecs N`
- 高吞吐场景增大 budget，低延迟场景减小并配合 busy polling

## HFT 关联

| 特性 | HFT 用途 |
|------|---------|
| SO_BUSY_POLL | 行情接收进程持续轮询，跳过中断唤醒延迟 |
| Threaded NAPI | NAPI 线程绑定到隔离 CPU，避免软中断争抢 |
| budget 调优 | 小 budget + busy poll = 最低收包延迟 |

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| NAPI 上下文 | 仅软中断 | 软中断 或 threaded NAPI |
| busy polling | 不存在 | SO_BUSY_POLL + NAPI_ID |
| budget 控制 | 固定 64 | 可调，配合 ethtool coalescing |
"""),

("lwn-articles-summary/02-page-pool.md", """# 02 — page_pool API：现代 Rx buffer 管理

> **对应 Rosen:** Ch1/Ch4（收包路径中 buffer 分配）
> **内核版本:** 4.18+（page_pool 核心库），广泛采用于 5.x 驱动

## 问题背景

传统收包路径中，每个 Rx buffer 需要分配一个 page：
- `alloc_page()` → 映射 DMA → 填充数据 → 传递给协议栈
- page 使用完后 `put_page()` 释放，下次收包再 `alloc_page()`
- 高 PPS 场景下，page 分配/释放成为瓶颈

## page_pool 解决方案

page_pool 是一个**页 recycling 池**：
- 预分配一批 page，收包时从池中取
- page 传递给协议栈时增加引用计数
- 协议栈释放 page 时回到池中（而非真正释放）
- 避免反复 `alloc_page()` / `put_page()`

```c
struct page_pool *pp;
struct page_pool_params params = {
    .order = 0,
    .flags = PP_FLAG_DMA_MAP,
    .pool_size = 256,
    .nid = NUMA_NO_NODE,
    .dev = &pdev->dev,
    .dma_dir = DMA_FROM_DEVICE,
};
pp = page_pool_create(&params);

// 收包时
page = page_pool_dev_alloc_pages(pp);
dma_addr = page_pool_get_dma_addr(page);

// 释放时（协议栈持有完毕）
page_pool_put_full_page(pp, page, false);
```

## 现代驱动采用情况

| 驱动 | 是否使用 page_pool | 内核版本 |
|------|-------------------|---------|
| mlx5（Mellanox） | 是 | 5.x+ |
| ice（Intel E810） | 是 | 5.x+ |
| stmmac（树莓派 5 网卡） | 是 | 5.x+ |
| virtio-net | 是 | 5.x+ |

## 与 XDP 的协同

page_pool 是 XDP 的基础设施：
- XDP 程序在 page_pool 分配的 page 上运行
- AF_XDP 零拷贝路径直接使用 page_pool 的 page
- XDP redirect 可以传递 page_pool 的 page 到其他设备

## HFT 关联

| 维度 | HFT 影响 |
|------|---------|
| 内存分配延迟 | 消除收包路径中的 alloc_page 开销 |
| NUMA 亲和性 | page_pool 可绑定 NUMA node，避免跨节点访问 |
| DMA 映射开销 | page_pool 缓存 DMA 映射，避免重复 map/unmap |

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| Rx buffer 分配 | alloc_page 每次分配 | page_pool recycling |
| DMA 映射 | 每次收包 map/unmap | 缓存映射 |
| XDP 支持 | 不存在 | page_pool 是 XDP 基础设施 |
"""),

("lwn-articles-summary/03-gro-gso.md", """# 03 — GRO/GSO 演进与性能

> **对应 Rosen:** Ch11（Layer 4，sk_buff 处理）
> **内核版本:** GRO 2.6.29+；GSO 更早；硬件 offload 持续演进

## GRO（Generic Receive Offload）

GRO 在收包路径将多个小包合并成一个大包：
- 减少协议栈处理次数（每个大包只走一次 IP/TCP 处理）
- 减少 sk_buff 分配数量
- 合并条件：同 flow、同 IP/TCP 头部、连续序列号

现代演进：
- `napi_gro_receive()` → `napi_gro_flush()` 路径优化
- GRO 可按协议禁用：`ethtool -K eth0 gro off`
- XDP 路径不经过 GRO（XDP 在 GRO 之前处理）

## GSO（Generic Segmentation Offload）

GSO 在发包路径将大包延迟分段：
- 协议栈构造一个大的 sk_buff（最多 64KB）
- 驱动或硬件负责实际分段
- 减少协议栈处理开销

现代演进：
- TSO（TCP Segmentation Offload）：硬件分段 TCP 大包
- GSO partial：部分硬件分段 + 部分软件分段
- UFO → USO（UDP Segmentation Offload）：5.x+ 支持 UDP 分段

## 性能权衡（HFT 视角）

| 机制 | 吞吐量 | 延迟 | HFT 建议 |
|------|--------|------|---------|
| GRO on | 高（合并包） | 增加延迟（等待合并窗口） | 行情接收关闭 GRO |
| GRO off | 低 | 最低延迟 | HFT 行情流推荐 |
| GSO/TSO on | 高（大包发送） | 发送节奏不可控 | 交易报文关闭，行情组播关闭 |
| GSO/TSO off | 低 | 每包独立发送 | 小交易报文推荐 |

## HFT 关联

- **行情接收**：GRO 会引入合并等待延迟（微秒级），HFT 应关闭 `ethtool -K eth0 gro off`
- **交易发送**：TSO 会将多个小报文合并发送，影响发送时机，HFT 应关闭 `ethtool -K eth0 tso off`
- **UDP GRO**：5.0+ 引入，组播行情批量接收可提升吞吐，但增加延迟
"""),

("lwn-articles-summary/04-sk-buff-xdp-buff.md", """# 04 — sk_buff → xdp_buff：收包路径分流

> **对应 Rosen:** Ch1/Ch11（sk_buff 是唯一数据结构）
> **内核版本:** xdp_buff 4.8+；xdp_frame 4.18+

## 传统收包路径（Rosen 3.x）

```
NIC DMA → alloc_page → NAPI poll → sk_buff alloc → 协议栈 → socket
```
- 每个收到的包都分配一个 `sk_buff`（约 256 字节）
- sk_buff 包含大量元数据（指针、协议头偏移、队列映射等）
- 分配 sk_buff 后才能传递给协议栈处理

## XDP 引入的数据路径分流

```
NIC DMA → page_pool alloc → XDP hook (xdp_buff) → 决策：
  ├─ XDP_PASS  → 分配 sk_buff → 协议栈 → socket（传统路径）
  ├─ XDP_DROP  → 直接丢弃，不分配 sk_buff
  ├─ XDP_TX    → 原路反弹发送
  ├─ XDP_REDIRECT → 转发到其他设备/CPUMAP/AF_XDP socket
  └─ XDP_ABORTED → 错误，丢弃
```

## xdp_buff vs sk_buff

| 维度 | sk_buff | xdp_buff |
|------|---------|---------|
| 分配时机 | 收到包后立即分配 | XDP 处理完才分配（PASS 时） |
| 大小 | ~256 字节元数据 | ~64 字节，轻量 |
| 数据访问 | 指针跳转多层 | 线性 data/data_end 指针 |
| 协议栈 | 必需 | 不经过协议栈 |
| 可修改性 | 可改头部但开销大 | 可原地改包内容 |

## 性能影响

XDP DROP 路径完全不分配 sk_buff：
- 传统路径：alloc_page + alloc sk_buff + 协议栈处理 → ~300 cycles
- XDP DROP：检查包头 → ~10 cycles
- 对 HFT 行情流中的垃圾包过滤非常有效

## HFT 关联

| 场景 | xdp_buff 优势 |
|------|-------------|
| 行情早过滤 | XDP 检查组播地址/端口，丢弃无关包，不分配 sk_buff |
| 行情早分类 | XDP 修改包的队列映射，引导到特定 CPU |
| AF_XDP 零拷贝 | xdp_buff 直接 redirect 到用户态，不经过协议栈 |
"""),

# --- 2. XDP (4) ---
("lwn-articles-summary/05-xdp-architecture.md", """# 05 — XDP 架构与 use case 全景

> **对应 Rosen:** 无（书出版时 XDP 不存在）
> **内核版本:** XDP hook 4.8+；AF_XDP 4.18+；XDP multi-attach 5.1+

## XDP 是什么

XDP（eXpress Data Path）是内核网络栈的**最早数据路径 hook**：
- 在驱动层、sk_buff 分配之前执行
- 以 eBPF 程序形式运行（JIT 编译为原生指令）
- 可以查看/修改包内容、丢弃、重定向

## 四种 XDP 模式

| 模式 | 挂载点 | 硬件要求 | 性能 |
|------|--------|---------|------|
| Native XDP | 驱动层（网卡驱动支持） | 驱动需实现 XDP hook | 最高 |
| Offloaded XDP | 网卡硬件（SmartNIC） | 网卡支持 eBPF 卸载 | 极致（不占 CPU） |
| Generic XDP | 协议栈入口（sk_buff 之后） | 无 | 最低（仍分配 sk_buff） |
| SKB-mode XDP | 类似 Generic | 无 | 低 |

## XDP 程序类型

```c
// 最简 XDP 程序：丢弃所有包
SEC("xdp")
int xdp_drop_all(struct xdp_md *ctx) {
    return XDP_DROP;
}

// 检查目标端口，丢弃非行情端口
SEC("xdp")
int xdp_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    if ((void*)(eth+1) > data_end) return XDP_DROP;
    if (eth->h_proto != htons(ETH_P_IP)) return XDP_PASS;
    // ... 检查 IP/TCP/UDP 头
    return XDP_PASS;
}
```

## XDP use case 全景

| 场景 | XDP 动作 | HFT 关联 |
|------|---------|---------|
| DDoS 防护 | XDP_DROP 丢弃攻击包 | 保护交易服务器 |
| 负载均衡 | XDP_REDIRECT 到后端 | 行情分发 |
| 包过滤 | XDP_DROP 无关包 | 行情流早过滤 |
| 协议预处理 | XDP_TX 响应 | ARP/ICMP 快速响应 |
| 监控统计 | XDP_PASS + 计数 | 延迟/丢包监控 |
| AF_XDP | XDP_REDIRECT 到用户态 | 内核旁路收包 |

## HFT 关联

XDP 是 HFT 在**不使用 DPDK 时的最佳内核态方案**：
- 行情组播流：XDP 过滤无关组播组，只放行目标行情
- 延迟监控：XDP 打时间戳，测量 NIC → 内核 → 用户态各段延迟
- AF_XDP：零拷贝将行情包送到用户态，性能接近 DPDK

## 与 DPDK 的定位区别

| 维度 | XDP | DPDK |
|------|-----|------|
| 运行位置 | 内核态 | 用户态 |
| 内核参与 | 是（但极早） | 否（完全旁路） |
| 部署复杂度 | 低（加载 BPF 程序） | 高（绑定 UIO/VFIO 驱动） |
| 灵活性 | 高（可与其他内核功能共存） | 低（网卡被独占） |
| 适用场景 | 中低频、需内核功能 | 超低延迟、co-location |
"""),

("lwn-articles-summary/06-af-xdp.md", """# 06 — AF_XDP：零拷贝到用户态

> **对应 Rosen:** 无
> **内核版本:** 4.18+（初始）；zero-copy 模式需驱动支持

## AF_XDP 是什么

AF_XDP（Address Family XDP）是一种 socket 类型，允许用户态程序直接从 XDP hook 接收包：
- XDP 程序将包 redirect 到 AF_XDP socket
- 用户态程序从共享内存环形缓冲区读取包
- 零拷贝模式下，page_pool 的 page 直接映射到用户态

## 架构

```
NIC DMA → page_pool → XDP hook → XDP_REDIRECT → AF_XDP socket
                                                    ↓
                                           UMEM（用户态共享内存）
                                           ↙              ↘
                                     FILL ring          RX ring
                                   (空闲 buffer)      (收到的包)

                                     TX ring          COMPLETION ring
                                   (待发送包)        (发送完成通知)
```

## 四个环形缓冲区

| Ring | 方向 | 作用 |
|------|------|------|
| FILL | 用户→内核 | 用户态提供空闲 buffer 给内核填充 |
| RX | 内核→用户 | 内核将收到的包放入 buffer，通知用户态 |
| TX | 用户→内核 | 用户态将待发送包放入 buffer |
| COMPLETION | 内核→用户 | 内核通知发送完成，buffer 可回收 |

## 两种模式

| 模式 | 机制 | 延迟 | 驱动要求 |
|------|------|------|---------|
| Copy mode | 内核拷贝包到 UMEM | 较高 | 所有 XDP 驱动 |
| Zero-copy mode | page_pool 直接映射 | 最低 | 驱动支持 zero-copy |

## 代码框架

```c
// 创建 AF_XDP socket
int sockfd = socket(AF_XDP, SOCK_RAW, 0);

// 注册 UMEM（共享内存区域）
struct xdp_umem_reg umem = {
    .addr = (uintptr_t)buffer,
    .len = BUFFER_SIZE,
    .chunk_size = 4096,
};
setsockopt(sockfd, SOL_XDP, XDP_UMEM_REG, &umem, sizeof(umem));

// 绑定到网卡+队列
struct sockaddr_xdp sxdp = {
    .sxdp_family = AF_XDP,
    .sxdp_ifindex = ifindex,
    .sxdp_queue_id = queue_id,
    .sxdp_flags = XDP_ZEROCOPY,  // 零拷贝模式
};
bind(sockfd, (struct sockaddr*)&sxdp, sizeof(sxdp));

// 用户态轮询收包
struct xdp_desc desc;
while (1) {
    while (xsk_ring_cons__has_data(&rx_ring)) {
        // 直接访问 UMEM 中的包数据，零拷贝
        void *pkt = xsk_umem__get_data(buffer, desc.addr);
        // 处理行情...
    }
}
```

## HFT 关联

| 维度 | AF_XDP 优势 |
|------|------------|
| 零拷贝 | 包数据不经过内核协议栈，直接到用户态 |
| 低延迟 | 接近 DPDK，但不需要独占网卡 |
| 内核共存 | 与其他内核网络功能（路由/TCP）共存 |
| 灵活切换 | 行情流走 AF_XDP，管理流走普通 socket |

## AF_XDP vs DPDK

| 维度 | AF_XDP | DPDK |
|------|--------|------|
| 零拷贝 | 是（page_pool 映射） | 是（大页 + VFIO） |
| 网卡独占 | 否（与其他队列共享） | 是 |
| CPU 占用 | 用户态轮询 | 用户态轮询 |
| 部署 | 加载 BPF + bind socket | 绑定 VFIO + 配 hugepage |
| 延迟 | 略高于 DPDK | 最低 |
| 适合 HFT | 中低频策略 | 超低延迟 co-location |
"""),

("lwn-articles-summary/07-xdp-redirect.md", """# 07 — XDP redirect 与 cpumap

> **对应 Rosen:** 无
> **内核版本:** XDP redirect 4.8+；CPUMAP 4.15+；DEVMAP 4.14+

## XDP redirect 概述

XDP 程序可以将包重定向到不同目标，而非简单 PASS/DROP：
- **CPUMAP**：将包分发到特定 CPU 核心处理
- **DEVMAP**：将包转发到另一个网卡
- **AF_XDP socket**：将包送到用户态
- **BPF map（全局）**：跨程序/跨 CPU 传递包

## CPUMAP：CPU 亲和性收包分发

传统 RPS 在软件层分发包到不同 CPU，但需要先分配 sk_buff。
CPUMAP 在 XDP 层（sk_buff 之前）分发：

```c
// BPF 程序：按 hash 将包分发到不同 CPU
SEC("xdp")
int xdp_cpumap(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    // 解析包头部计算 hash
    uint32_t cpu = hash % NUM_CPUS;
    return bpf_redirect_map(&cpumap, cpu, 0);  // XDP_REDIRECT
}
```

CPUMAP 的工作原理：
1. XDP 程序调用 `bpf_redirect_map()` 将包放入目标 CPU 的队列
2. 目标 CPU 的 kthread 从队列取出包
3. 构造 sk_buff，送入协议栈（或再次通过 BPF 处理）

| 特性 | RPS（传统） | CPUMAP（XDP） |
|------|------------|---------------|
| 分发时机 | sk_buff 分配后 | sk_buff 分配前 |
| CPU 开销 | 需分配 sk_buff | 轻量，延迟分配 |
| 灵活性 | hash 策略固定 | BPF 程序自定义 |

## DEVMAP：网卡间转发

```c
SEC("xdp")
int xdp_redirect_dev(struct xdp_md *ctx) {
    return bpf_redirect_map(&devmap, target_ifindex, 0);
}
```
- 可实现内核态 L2 转发（替代部分 bridge 功能）
- 性能远高于传统转发（不经过协议栈）

## HFT 关联

| 场景 | redirect 类型 | 用途 |
|------|-------------|------|
| 行情多核分发 | CPUMAP | 不同行情流分发到不同 CPU，各自独立处理 |
| 行情镜像 | DEVMAP | 将行情复制转发到监控/录制服务器 |
| 行情旁路 | AF_XDP | 将行情包送到用户态交易进程 |
| 跨网卡转发 | DEVMAP | 交易报文从内网网卡转发到交易所网卡 |
"""),

("lwn-articles-summary/08-xdp-vs-dpdk.md", """# 08 — XDP vs DPDK：内核旁路两条路

> **对应 Rosen:** 无
> **内核版本:** XDP 4.8+；DPDK 用户态

## 两条旁路路线

| 维度 | XDP（内核态旁路） | DPDK（用户态旁路） |
|------|------------------|-------------------|
| **核心理念** | 在内核最早点处理，跳过协议栈 | 完全绕过内核，用户态驱动 |
| **运行位置** | 内核态（驱动层） | 用户态 |
| **网卡控制** | 内核驱动管理，XDP 挂载 BPF 程序 | 用户态驱动（UIO/VFIO），内核放弃控制 |
| **内存管理** | page_pool（内核管理） | hugepage（用户态管理） |
| **零拷贝** | AF_XDP zero-copy（page_pool 映射） | RTE buffer + hugepage |
| **CPU 占用** | NAPI 轮询 + BPF（可配置） | 100% 轮询（DPDK polling mode） |
| **中断** | 可关闭（busy poll） | 完全关闭 |
| **协议栈共存** | 是（非 XDP 路径仍走协议栈） | 否（网卡被 DPDK 独占） |
| **部署复杂度** | 低（加载 BPF 程序） | 高（绑定驱动 + hugepage + NUMA） |
| **调试工具** | 内核工具（bpftool/xdp-tools） | DPDK 工具（testpmd/dpdk-proc-info） |
| **生态** | 内核主线，跟随内核更新 | 独立项目，需适配内核版本 |

## 性能对比（典型值）

| 指标 | XDP | DPDK | 差距 |
|------|-----|------|------|
| 收包延迟 | ~100-200 ns | ~50-100 ns | DPDK 快 2x |
| 包处理速率 | ~24 Mpps/core | ~40+ Mpps/core | DPDK 快 1.5-2x |
| CPU 占用 | 可配置 | 100% | XDP 灵活 |
| 额外内存 | page_pool（少量） | hugepage（GB 级） | XDP 少 |

## HFT 选型决策

```
需要极致延迟（< 1μs）？
  ├─ 是 → co-location 环境 → DPDK
  └─ 否 → 需要内核功能（路由/TCP/安全）？
            ├─ 是 → XDP + AF_XDP
            └─ 否 → 延迟要求 < 5μs？
                      ├─ 是 → DPDK（非 co-location 也可）
                      └─ 否 → XDP（性价比最高）
```

## 混合方案

部分 HFT 系统使用混合架构：
- **行情接收**：DPDK（超低延迟，网卡独占）
- **管理通道**：内核协议栈（SSH/监控/控制）
- **行情分发**：XDP CPUMAP（在内核层分发到非 DPDK 网卡）
- 需要双网卡：一张 DPDK 独占，一张内核管理

## 常见误区

| 误区 | 事实 |
|------|------|
| XDP 一定能替代 DPDK | 不一定，co-location 场景 DPDK 延迟优势明显 |
| DPDK 一定比 XDP 快 | 非零拷贝 AF_XDP 比 DPDK 慢，但 zero-copy 差距缩小 |
| XDP 不需要 CPU | Native XDP 仍需要 CPU 轮询（NAPI 驱动） |
| DPDK 不需要内核 | DPDK 需要 UIO/VFIO 内核模块，只是数据路径旁路 |
"""),

# --- 3. eBPF 网络 (3) ---
("lwn-articles-summary/09-tc-bpf.md", """# 09 — tc-BPF：流量控制中的 eBPF

> **对应 Rosen:** Ch6（Advanced Routing）/ Ch9（Netfilter）
> **内核版本:** tc-BPF cls 4.1+；direct-action 4.1+

## tc-BPF 是什么

Linux Traffic Control（tc）子系统支持用 eBPF 程序做包分类和动作：
- **cls_bpf**：BPF 分类器，替代 u32/fw 等 传统分类器
- **direct-action**：BPF 程序直接返回动作（TC_ACT_OK/SHOT/REDIRECT），不需要单独的 filter action 模块

## 挂载点

| 挂载点 | 方向 | 作用 |
|--------|------|------|
| tc ingress | 收包入口 | 在协议栈之前分类/丢弃/修改包 |
| tc egress | 发包出口 | 在 qdisc 之后、驱动之前分类/修改包 |

## tc ingress vs XDP

| 维度 | XDP | tc ingress |
|------|-----|-----------|
| 挂载位置 | 驱动层（sk_buff 之前） | 协议栈入口（sk_buff 之后） |
| 数据结构 | xdp_buff（轻量） | sk_buff（完整元数据） |
| 可访问信息 | 包内容 + RX 队列索引 | 包内容 + sk_buff 全部元数据 |
| 能力 | 丢/转/改包 | 丢/转/改包 + 可设置 skb mark/priority |
| 性能 | 最高 | 略低（已分配 sk_buff） |

## 使用示例

```bash
# 加载 tc-BPF 程序到网卡 ingress
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj filter.o sec tc-ingress

# 查看
tc filter show dev eth0 ingress
```

```c
// tc-BPF 程序：按目标端口标记包
SEC("tc-ingress")
int tc_ingress(struct __sk_buff *skb) {
    void *data_end = (void *)(long)skb->data_end;
    void *data = (void *)(long)skb->data;
    struct ethhdr *eth = data;
    // ... 解析 IP/TCP 头
    if (tcp->dest == htons(9090)) {
        skb->mark = 0x9090;  // 标记行情流
        return TC_ACT_OK;
    }
    return TC_ACT_OK;  // 放行
}
```

## HFT 关联

| 场景 | tc-BPF 用途 |
|------|------------|
| 行情流标记 | 按 DSCP/端口设置 skb->mark，配合路由策略 |
| 发包控制 | tc egress 限制非交易流量带宽 |
| 包丢弃 | 丢弃不符合规则的包，在协议栈前拦截 |
| 延迟测量 | tc-BPF 打时间戳到 skb->cb |
"""),

("lwn-articles-summary/10-xdp-bpf.md", """# 10 — XDP-BPF：收包路径中的 eBPF

> **对应 Rosen:** 无
> **内核版本:** XDP BPF 4.8+

## XDP-BPF vs tc-BPF

| 维度 | XDP-BPF | tc-BPF |
|------|---------|--------|
| 挂载位置 | 驱动层（sk_buff 之前） | tc ingress/egress（sk_buff 之后） |
| 数据结构 | xdp_buff | sk_buff |
| 能修改 skb 元数据 | 否（无 sk_buff） | 是（mark/priority/queue_mapping） |
| 性能 | 最高 | 次之 |
| 适用场景 | 早过滤/早丢弃/早分类 | 需要 skb 元数据的场景 |

## XDP-BPF 程序能做什么

1. **读取/修改包内容**：直接操作 xdp_buff 的 data/data_end
2. **丢弃包**：返回 XDP_DROP，不分配 sk_buff
3. **重定向**：CPUMAP / DEVMAP / AF_XDP
4. **反弹发送**：XDP_TX（修改 MAC 后原路返回）
5. **修改包**：增删头部（bpf_xdp_adjust_head）

## 实际 HFT 应用：行情组播早过滤

```c
SEC("xdp")
int xdp_md_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    if ((void*)(eth + 1) > data_end) return XDP_DROP;
    if (eth->h_proto != bpf_htons(ETH_P_IP)) return XDP_PASS;

    struct iphdr *ip = (void*)(eth + 1);
    if ((void*)(ip + 1) > data_end) return XDP_DROP;

    // 只放行特定组播地址的行情流
    if (ip->daddr == bpf_htonl(0xe1000001)) {  // 225.0.0.1
        return XDP_PASS;  // 目标行情流
    }
    return XDP_DROP;  // 丢弃非行情组播
}
```

## 性能数据（参考）

| 操作 | 延迟（cycles） |
|------|---------------|
| XDP DROP（空程序） | ~10 |
| XDP PASS（空程序） | ~20 |
| XDP + 包头解析 | ~50-100 |
| sk_buff 分配 + 协议栈 | ~300+ |

## HFT 关联

XDP-BPF 是 HFT 在内核态做**最早点包处理**的唯一方案：
- 比 tc-BPF 更早（不分配 sk_buff）
- 比 Netfilter 更早（不经过协议栈）
- 可与 AF_XDP 配合实现接近 DPDK 的零拷贝路径
"""),

("lcp-articles-summary/11-cgroup-bpf.md", """# 11 — cgroup-BPF：容器网络隔离

> **对应 Rosen:** 无
> **内核版本:** cgroup BPF 4.10+；SOCK_ADDR 4.18+

## cgroup-BPF 是什么

cgroup-BPF 允许在 cgroup 级别挂载 BPF 程序，对该 cgroup 内所有进程的网络操作进行过滤/修改：

| 程序类型 | 挂载点 | 作用 |
|---------|--------|------|
| BPF_CGROUP_INET_INGRESS | cgroup ingress | 过滤该 cgroup 进程收到的包 |
| BPF_CGROUP_INET_EGRESS | cgroup egress | 过滤该 cgroup 进程发出的包 |
| BPF_CGROUP_SOCK_ADDR | connect/bind | 拦截/修改 connect/bind 调用 |
| BPF_CGROUP_SOCK_ops | socket 操作 | 监控/修改 socket 状态 |

## 与 XDP-BPF / tc-BPF 的区别

| 维度 | XDP-BPF | tc-BPF | cgroup-BPF |
|------|---------|--------|-----------|
| 作用域 | 全局（网卡级） | 全局（网卡级） | cgroup 级（进程组） |
| 粒度 | 每个包 | 每个 sk_buff | 每个进程的网络操作 |
| 能力 | 包过滤/重定向 | 包分类/标记 | socket 级过滤/修改 |

## HFT 关联

cgroup-BPF 在 HFT 中主要用于**进程隔离和资源控制**：
- 交易进程和行情进程在不同 cgroup，各自有独立网络策略
- 限制非交易进程的网络带宽
- 监控每个进程的 socket 状态（重传/RTT 等）

> 注：cgroup-BPF 不是 HFT 网络路径的主力工具，主要用于运维/隔离层面。
"""),

# --- 4. nftables (2) ---
("lwn-articles-summary/12-nftables.md", """# 12 — nftables 架构与迁移

> **对应 Rosen:** Ch9（Netfilter/iptables）
> **内核版本:** nftables 3.13+（初始）；4.1+（用户态工具成熟）

## 为什么需要 nftables

iptables 的问题：
- 每个 table/chain 是独立的内核模块（filter/nat/mangle/raw）
- 规则匹配线性扫描，大量规则时性能差
- IPv4 和 IPv6 需要两套规则（iptables / ip6tables）
- 每个匹配条件（-m tcp / -m state）是独立模块

## nftables 架构

nftables 统一了 Netfilter 前端：
- **统一语法**：IPv4/IPv6/ARP/Bridge 一套规则
- **虚拟机**：规则编译为字节码，在内核 nft VM 中执行
- **集合和映射**：原生支持 IP 集合，无需额外 ipset
- **无状态表**：table/chain 由用户态定义，不需要内核模块

## nftables vs iptables

| 维度 | iptables | nftables |
|------|---------|---------|
| 语法 | -A INPUT -p tcp --dport 80 -j ACCEPT | add rule inet filter input tcp dport 80 accept |
| 地址族 | ipv4/ip6 分开 | inet 统一（同时匹配 v4/v6） |
| 规则集 | 不支持 | 原生支持（集合/映射/区间） |
| 性能 | 线性扫描 | 可优化（集合用哈希/区间树） |
| 表/链 | 内核预定义 | 用户自定义 |
| 模块 | 每个匹配条件一个内核模块 | nft VM 统一执行 |

## 迁移示例

```bash
# iptables 规则
iptables -A INPUT -p tcp --dport 9090 -s 10.0.0.0/24 -j ACCEPT

# 等效 nftables 规则
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept

# 集合用法（替代 ipset）
nft add set inet filter whitelist { type ipv4_addr \; flags interval \; }
nft add element inet filter whitelist { 10.0.0.0/24, 192.168.1.0/24 }
nft add rule inet filter input ip saddr @whitelist accept
```

## HFT 关联

| 场景 | nftables 用途 |
|------|-------------|
| 行情源白名单 | 集合管理交易所 IP，一条规则匹配 |
| 交易端口保护 | 只允许特定 IP 连接交易端口 |
| 速率限制 | limit 规则限制 ICMP/DNS 等非交易流量 |
| 兼容性 | iptables-nft 兼容层，旧脚本可平滑迁移 |
"""),

("lwn-articles-summary/13-nftables-vs-bpf.md", """# 13 — nftables 与 eBPF 的关系

> **对应 Rosen:** 无
> **内核版本:** nftables 3.13+；eBPF 网络 4.x+

## 两种内核包过滤机制

| 维度 | nftables | eBPF（XDP/tc-BPF） |
|------|---------|-------------------|
| 设计目标 | 通用防火墙/NAT | 可编程数据路径 |
| 执行位置 | Netfilter hook（协议栈内） | XDP hook 或 tc 层 |
| 编程模型 | 声明式规则（nft 语法） | 命令式程序（C→BPF） |
| 灵活性 | 规则匹配（有限） | 任意逻辑（图灵完备） |
| 性能 | 比 iptables 快，但比 XDP 慢 | XDP 最快，tc-BPF 次之 |
| 适用场景 | 防火墙/NAT/安全策略 | 高性能包处理/过滤/监控 |

## 何时用哪个

| 场景 | 推荐 | 原因 |
|------|------|------|
| 防火墙规则（端口/IP过滤） | nftables | 声明式简单，维护方便 |
| NAT/路由 | nftables | Netfilter NAT 成熟稳定 |
| 超低延迟包过滤 | XDP-BPF | sk_buff 之前处理 |
| 行情早过滤/早丢弃 | XDP-BPF | 不分配 sk_buff |
| 包标记/分类 | tc-BPF | 可设置 skb 元数据 |
| 速率限制 | nftables 或 tc-BPF | nftables limit 简单，tc-BPF 更灵活 |
| 包内容修改 | XDP-BPF | 可增删头部 |
| 可观测/监控 | eBPF（tracing） | 可 hook 任意内核函数 |

## HFT 实践

HFT 系统通常两者都用：
- **nftables**：基础防火墙（管理通道保护、白名单、NAT）
- **XDP-BPF**：行情流早过滤（组播地址过滤、端口过滤）
- **tc-BPF**：发包控制（交易流标记、带宽控制）

```
行情包到达 → XDP-BPF（早过滤）→ 协议栈 → nftables（安全规则）→ socket
交易包发送 → socket → tc-BPF（标记）→ qdisc → nftables（NAT）→ 驱动
```
"""),

# --- 5. io_uring 网络 (2) ---
("lwn-articles-summary/14-io-uring-net.md", """# 14 — io_uring 网络收发接口

> **对应 Rosen:** 无
> **内核版本:** io_uring 5.1+；网络操作 5.5+；multishot 5.19+

## io_uring 是什么

io_uring 是 Linux 的异步 IO 框架：
- 两个环形队列：SQ（提交队列）+ CQ（完成队列）
- 用户态程序通过共享内存提交 IO 请求，无需系统调用
- 内核异步完成后写入 CQ，用户态轮询读取

## io_uring 网络操作

| 操作 | opcode | 内核版本 | 说明 |
|------|--------|---------|------|
| accept | IORING_OP_ACCEPT | 5.5 | 异步 accept() |
| recv/recvmsg | IORING_OP_RECV / RECVMSG | 5.6 | 异步接收 |
| send/sendmsg | IORING_OP_SEND / SENDMSG | 5.6 | 异步发送 |
| connect | IORING_OP_CONNECT | 5.6 | 异步 connect |
| multishot accept | IORING_OP_ACCEPT + multishot | 5.19 | 一次提交多次完成 |
| sendzc | IORING_OP_SEND_ZC | 6.0 | 零拷贝发送 |

## io_uring 网络收发流程

```
用户态:
  1. 准备 recv SQE → 写入 SQ ring
  2. io_uring_enter() 通知内核（或 SQPOLL 模式下内核自旋轮询）
  3. 轮询 CQ ring 等待完成事件

内核态:
  1. 从 SQ ring 读取 recv 请求
  2. 调用 socket 的 recvmsg 回调
  3. 数据就绪后写入 CQ ring
```

## SQPOLL 模式（内核轮询线程）

io_uring 可以创建一个内核线程持续轮询 SQ：
- 用户态不需要调用 `io_uring_enter()`，零系统调用提交
- 适合 HFT 低延迟场景（但消耗一个 CPU 核心）
- `IORING_SETUP_SQPOLL` flag 启用

## HFT 关联

| 维度 | io_uring 优势 |
|------|-------------|
| 系统调用开销 | 零（SQPOLL 模式）或 1 次/批（非 SQPOLL） |
| 批量操作 | 多个 recv/send 一次提交 |
| 异步等待 | 不阻塞，CQ 通知完成 |
| 零拷贝发送 | SEND_ZC（6.0+） |

## io_uring vs epoll

| 维度 | epoll | io_uring |
|------|-------|---------|
| 事件模型 | 事件通知（就绪后 read/write） | 完成模型（直接完成操作） |
| 系统调用 | epoll_wait + read/write | io_uring_enter（或 SQPOLL 零调用） |
| 数据拷贝 | read/write 仍需拷贝 | 可配合零拷贝（SEND_ZC） |
| 批量 | 不支持 | 一次提交多个操作 |
| 复杂度 | 低 | 中 |
"""),

("lwn-articles-summary/15-io-uring-vs-epoll.md", """# 15 — io_uring vs epoll：性能对比

> **对应 Rosen:** 无
> **内核版本:** epoll 2.6+；io_uring 5.1+

## 事件模型 vs 完成模型

**epoll（事件模型）：**
```
epoll_wait() → 通知 socket 可读 → recvmsg() → 数据就绪
     ↑ 系统调用 1              ↑ 系统调用 2
```
- 两次系统调用：epoll_wait + recvmsg
- epoll 只通知"可读"，实际读取仍需调用 recv

**io_uring（完成模型）：**
```
提交 recv SQE → 内核异步完成 → CQ 通知数据就绪
  ↑ 0 次系统调用（SQPOLL）或 1 次
```
- 一次提交包含完整操作（recv），内核完成后通知
- SQPOLL 模式下零系统调用

## 性能对比（单连接）

| 指标 | epoll + recvmsg | io_uring (SQPOLL) |
|------|----------------|-------------------|
| 系统调用数 | 2（epoll_wait + recv） | 0（SQPOLL 自旋） |
| 延迟 | ~1-2 μs | ~0.5-1 μs |
| CPU 占用 | 事件驱动（可休眠） | 100%（SQPOLL 线程） |

## 性能对比（多连接）

| 连接数 | epoll 优势 | io_uring 优势 |
|--------|-----------|-------------|
| < 100 | 简单够用 | 批量提交减少系统调用 |
| 100-10000 | 仍可接受 | 批量优势明显 |
| > 10000 | 性能下降 | 优势扩大 |

## HFT 适用性

| 场景 | 推荐 | 原因 |
|------|------|------|
| 行情接收（少量连接） | io_uring SQPOLL | 零系统调用，最低延迟 |
| 交易发送（少量连接） | io_uring SEND_ZC | 零拷贝 + 异步 |
| 管理通道（多连接） | epoll | 简单，不需要 100% CPU |
| 兼容性要求 | epoll | io_uring 需 5.5+ 内核 |

## 代码复杂度对比

epoll 代码更简单，io_uring 需要：
- 初始化 io_uring 实例 + 注册 buffer
- 管理 SQE/CQE 生命周期
- 处理 buffer recycling

> HFT 建议：行情/交易路径用 io_uring（SQPOLL + 零拷贝），管理通道用 epoll。
"""),

# --- 6. TCP/UDP 性能 (5) ---
("lwn-articles-summary/16-msg-zerocopy.md", """# 16 — MSG_ZEROCOPY：零拷贝发送

> **对应 Rosen:** Ch11（sendmsg 只有拷贝模式）
> **内核版本:** MSG_ZEROCOPY 4.14+

## 传统发送路径

```
用户态 buffer → copy_from_user() → 内核 skb data → 驱动 → NIC DMA
```
- sendmsg() 将用户态数据拷贝到内核 sk_buff
- 大数据量时拷贝开销显著（memcpy 占 CPU）

## MSG_ZEROCOPY 机制

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt));

// 发送时标记零拷贝
sendmsg(sockfd, &msg, MSG_ZEROCOPY);
```

工作原理：
1. 内核不拷贝用户态数据，而是将用户态 page 映射到 sk_buff
2. NIC DMA 直接从用户态 page 读取数据
3. 发送完成后内核通知用户态（通过 errqueue）
4. 用户态收到通知后才能修改/释放 buffer

## 通知机制

零拷贝发送是异步的，内核通过 `MSG_ERRQUEUE` 通知完成：
```c
struct msghdr msg = {};
char control[CMSG_SPACE(sizeof(struct sock_extended_err))];
msg.msg_control = control;
msg.msg_controllen = sizeof(control);
recvmsg(sockfd, &msg, MSG_ERRQUEUE);
// 检查完成通知，可以安全释放 buffer
```

## 性能提升

| 数据量 | 传统 sendmsg | MSG_ZEROCOPY | 提升 |
|--------|-------------|-------------|------|
| 1 KB | ~1 μs | ~1.5 μs | 更慢（通知开销） |
| 64 KB | ~5 μs | ~1.5 μs | 3x |
| 1 MB | ~80 μs | ~3 μs | 25x |

> 小包零拷贝反而更慢（通知开销 > 拷贝开销），HFT 交易报文通常很小，不一定适合。

## HFT 关联

| 场景 | MSG_ZEROCOPY 适用性 |
|------|---------------------|
| 交易报文（< 1KB） | 不推荐（通知开销 > 拷贝开销） |
| 行情转发（大包） | 推荐（减少大包拷贝） |
| 批量发送 | 配合 io_uring SEND_ZC 效果更好 |
"""),

("lwn-articles-summary/17-tcp-zero-copy-recv.md", """# 17 — TCP zero-copy 接收

> **对应 Rosen:** Ch11（recvmsg 只有拷贝模式）
> **内核版本:** TCP zero-copy receive 5.0+（tcp_recvmsg MSG_ZEROCOPY 实为 mmap 方案）

## 传统接收路径

```
NIC DMA → 内核 page → copy_to_user() → 用户态 buffer
```
- recvmsg() 将内核 sk_buff 数据拷贝到用户态
- 高吞吐场景拷贝开销大

## TCP zero-copy 接收方案

Linux 提供两种 TCP 零拷贝接收方式：

### 方案 1：TCP mmap（`TCP_ZEROCOPY_RECEIVE`，4.18+）

```c
struct tcp_zerocopy_receive zc = {
    .address = (uintptr_t)mapped_buffer,
    .length = buffer_size,
};
setsockopt(sockfd, IPPROTO_TCP, TCP_ZEROCOPY_RECEIVE,
           &zc, sizeof(zc));
```
- 内核将 TCP 接收队列中的 page 映射到用户态地址空间
- 用户态直接读取映射内存，无需拷贝
- 读取完后通知内核（offset 更新）

### 方案 2：io_uring registered buffers（5.7+）

```c
// 注册用户态 buffer 到 io_uring
struct iovec iov = { .iov_base = buf, .iov_len = buf_size };
io_uring_register_buffers(ring, &iov, 1);

// 提交 recv 直接写入注册 buffer
io_uring_prep_recv(sqe, sockfd, buf, buf_size, 0);
```
- 内核直接将数据写入预注册的用户态 buffer
- 无需拷贝（DMA → 用户态 page）

## 性能对比

| 方案 | 拷贝次数 | 延迟 | 复杂度 |
|------|---------|------|--------|
| 传统 recvmsg | 1（内核→用户） | 基准 | 低 |
| TCP_ZEROCOPY_RECEIVE | 0（mmap） | 略低 | 中 |
| io_uring + registered buf | 0（DMA 直达） | 最低 | 高 |

## HFT 关联

| 场景 | 推荐方案 |
|------|---------|
| 行情接收（小包） | 传统 recvmsg（小包拷贝开销可忽略） |
| 行情转发（大包） | TCP_ZEROCOPY_RECEIVE 或 io_uring |
| AF_XDP | 最优（根本不经过 TCP 协议栈） |
"""),

("lwn-articles-summary/18-so-reuseport.md", """# 18 — SO_REUSEPORT：多进程负载均衡

> **对应 Rosen:** 无
> **内核版本:** SO_REUSEPORT 3.9+；eBPF attach 4.6+

## 传统多进程监听同一端口

- `SO_REUSEADDR`：多个 socket 绑定同一地址，但只有最后一个能 accept
- 需要一个 master 进程 accept 后分发给 worker
- master 成为瓶颈

## SO_REUSEPORT

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
listen(sockfd, backlog);
```

- 多个进程/线程各自创建 socket，都绑定同一地址+端口
- 内核将连接**均匀分发**到各 socket
- 每个进程独立 accept，无 master 瓶颈

## 分发算法

| 版本 | 算法 | 特点 |
|------|------|------|
| 3.9-4.5 | hash(4-tuple) % N | 同一连接总是到同一 socket |
| 4.6+ | eBPF 自定义 | 可按任意逻辑分发（CPU 亲和性等） |

## eBPF 分发（4.6+）

```c
SEC("sk_reuseport")
int select_socket(struct sk_reuseport_md *ctx) {
    // 按 CPU 亲和性选择 socket
    int cpu = bpf_get_smp_processor_id();
    return cpu % NUM_SOCKETS;
}
```

## HFT 关联

| 场景 | SO_REUSEPORT 用途 |
|------|-------------------|
| 行情组播接收 | 多进程各自绑定同一组播组，各自独立处理 |
| TCP 行情连接 | 多 worker 线程各自 accept，避免 master 瓶颈 |
| CPU 亲和性 | eBPF 按当前 CPU 选择 socket，数据和处理在同核 |
"""),

("lwn-articles-summary/19-tcp-internals.md", """# 19 — TCP 内部优化（TSO / pacing / RACK）

> **对应 Rosen:** Ch11（TCP 基础实现）
> **内核版本:** TSO 很早；pacing 3.12+；RACK 4.9+；TCP internal offload 持续演进

## TSO（TCP Segmentation Offload）

内核构造一个大的 TCP 段（最多 64KB），由硬件负责分段：
- 减少协议栈处理次数
- 减少驱动 DMA 描述符数量
- 现代网卡普遍支持

HFT 注意：TSO 会影响发送时机——内核等待凑够大段才发送，增加延迟。
```bash
# 关闭 TSO（降低发送延迟）
ethtool -K eth0 tso off
```

## Pacing（发送节奏控制）

传统 TCP 依赖拥塞窗口（cwnd）控制发送量，但不控制发送节奏：
- cwnd 允许发送 N 个包，内核可能瞬间全部发出（burst）
- burst 导致交换机队列拥塞 → 尾延迟增加

Pacing 将 cwnd 均匀分布在 RTT 内发送：
- `sk_pacing_rate`：每个 socket 的发送速率
- 内核用 fq qdisc 或 sch_fq 实现 pacing
- 3.12+ 默认为每个 TCP 流设置 pacing rate

HFT 影响：交易报文通常很小，pacing 影响不大。但行情转发流受 pacing 影响。

## RACK（Recent ACKnowledgement，4.9+）

RACK 改进 TCP 丢包检测：
- 传统：3 个重复 ACK 或超时 → 判定丢包
- RACK：根据最近 ACK 的时间戳推断哪些包可能丢失
- 更快检测丢包，减少等待时间
- 已成为默认丢包检测算法（替代 SACK + FACK）

## 其他现代 TCP 优化

| 特性 | 内核版本 | 作用 |
|------|---------|------|
| TCP Fast Open | 3.6+ | 首包携带数据，省一个 RTT |
| TCP repair | 3.5+ | 迁移 TCP 连接（容器/进程迁移） |
| TLS offload | 4.13+ | 网卡硬件 TLS 加解密 |
| TCP_AUTHOPT | 5.15+ | TCP MD5 替代（AO 选项） |

## HFT 关联

| 特性 | HFT 建议 |
|------|---------|
| TSO | 交易报文关闭（降低延迟） |
| Pacing | 交易流影响小；行情转发流注意 |
| RACK | 保持默认（更快丢包检测） |
| TCP Fast Open | 交易连接建立时可启用 |
| TLS offload | 加密交易流时可用 |
"""),

("lwn-articles-summary/20-udp-gro.md", """# 20 — UDP GRO：批量接收

> **对应 Rosen:** Ch11（UDP 无 GRO）
> **内核版本:** UDP GRO 5.0+

## 背景

组播行情通常使用 UDP：
- 每个行情更新一个 UDP 包
- 高频行情流每秒数万包
- 每个包小（几百字节到 1KB）
- 高 PPS 导致每包处理开销大

## UDP GRO 机制

GRO（Generic Receive Offload）原只支持 TCP，5.0+ 扩展到 UDP：
- 收包路径将同 flow 的多个 UDP 包合并成一个大包
- 协议栈只处理一次（而非 N 次）
- `recvmsg()` 一次读取合并后的大包

```c
// 接收 UDP GRO 合并包
struct msghdr msg = {};
char buf[65535];
struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
msg.msg_iov = &iov;
msg.msg_iovlen = 1;

int n = recvmsg(sockfd, &msg, 0);
// n 可能是多个原始包合并后的大小
// 需要解析 GSO 段（msg.msg_flags & MSG_EOR 标记分段）
```

## UDP GRO forwarding

合并后的 UDP 包可以转发到其他 socket 或网卡：
- `UDP_GRO` → `sendmsg(MSG_ZEROCOPY)` → GSO 发送
- 减少转发路径处理开销

## 性能影响

| 指标 | 无 UDP GRO | UDP GRO |
|------|-----------|---------|
| PPS 处理能力 | ~3 Mpps/core | ~8 Mpps/core |
| 延迟 | 基准 | 增加合并窗口延迟 |
| CPU 占用 | 高 | 低 |

## HFT 关联

| 场景 | UDP GRO 适用性 |
|------|---------------|
| 行情接收（需最低延迟） | 不推荐（合并窗口增加延迟） |
| 行情转发/录制（吞吐优先） | 推荐（减少 CPU 开销） |
| 组播行情中继 | 推荐 |

> HFT 交易路径不建议开 UDP GRO（延迟优先）。行情录制/转发等非关键路径可开（吞吐优先）。
"""),
]

# Fix typo: one file has wrong prefix
LWN[10] = ("lwn-articles-summary/11-cgroup-bpf.md", LWN[10][1].replace("lcp-articles-summary", "lwn-articles-summary"))

# ============================================================
# Part 2: Kernel Docs (12 files)
# ============================================================

KDOCS = [
("kernel-docs-summary/01-napi.md", """# 01 — Documentation/networking/napi.rst

> **对应 Rosen:** Ch1（NAPI 基础）/ Ch14（高级主题）
> **内核源码路径:** `Documentation/networking/napi.rst`

## 文档概述

内核 NAPI 官方文档，描述现代 NAPI 的完整工作流程和驱动 API。

## 核心内容

### NAPI 实例生命周期

```
napi_enable() → POLL 状态 → napi_schedule() → 轮询 → napi_complete()
     ↑                    ↑                         |
     └────────────────────┴─────────────────────────┘
```

### 关键 API

| API | 作用 |
|-----|------|
| `netif_napi_add()` | 注册 NAPI 实例（驱动初始化时） |
| `napi_enable()` | 启用 NAPI |
| `napi_schedule()` | 请求调度 NAPI 轮询（中断处理中调用） |
| `napi_poll()` | 驱动提供的轮询回调 |
| `napi_complete_done()` | 轮询完成，重新开中断 |
| `napi_disable()` | 禁用 NAPI |

### threaded NAPI（5.11+）

```
# 启用
echo 1 > /sys/class/net/eth0/threaded

# NAPI 线程出现在 ps
ps aux | grep napi
# napi/eth0-3   ...
```

### GRO 与 NAPI 的关系

- `napi_gro_receive()`：驱动收包后通过 NAPI 传入 GRO
- GRO 在 NAPI 轮询期间合并包
- `napi_gro_flush()`：NAPI 轮询结束时刷新未合并的包

## HFT 要点

- SO_BUSY_POLL 需要驱动支持 NAPI ID（`napi_id`）
- `ethtool -C` 调节 NAPI 的中断合并参数（rx-usecs / rx-frames）
- threaded NAPI 绑定 CPU：`taskset -c <core> $(pgrep napi/eth0)`

## 与 Rosen 3.x 的差异

- Rosen 描述的 NAPI 是 2.6 时代基础版本
- 现代 NAPI 新增：threaded mode、busy polling、GRO 集成、page_pool 集成
"""),

("kernel-docs-summary/02-page-pool.md", """# 02 — Documentation/networking/page_pool.rst

> **对应 Rosen:** Ch1（Rx buffer 分配）
> **内核源码路径:** `Documentation/networking/page_pool.rst`

## 文档概述

page_pool API 官方文档，描述 Rx buffer 的 recycling 池机制。

## 核心内容

### page_pool 工作流程

```
1. 驱动初始化：page_pool_create(&params)
2. 收包：page = page_pool_dev_alloc_pages(pp)
3. DMA 映射：page_pool_get_dma_addr(page)
4. 传递给协议栈（增加引用计数）
5. 协议栈释放：page_pool_put_full_page(pp, page, false) → 回到池中
```

### 关键配置参数

| 参数 | 说明 |
|------|------|
| `pool_size` | 池中 page 数量 |
| `order` | page order（0 = 4KB，1 = 8KB...） |
| `flags` | PP_FLAG_DMA_MAP / PP_FLAG_DMA_SYNC_DEV |
| `nid` | NUMA node 绑定 |
| `dma_dir` | DMA 方向 |

### 与 XDP 的关系

- XDP 程序运行在 page_pool 分配的 page 上
- XDP PASS → page 传递给 sk_buff（不释放）
- XDP DROP → page 直接回收到池中
- AF_XDP → page 映射到用户态 UMEM

### 性能数据

| 场景 | alloc_page | page_pool | 提升 |
|------|-----------|-----------|------|
| 1 Mpps | ~300 cycles/pkt | ~20 cycles/pkt | 15x |
| 10 Mpps | 瓶颈 | 轻松应对 | — |

## HFT 要点

- page_pool 消除收包路径的动态内存分配
- NUMA 绑定确保 Rx buffer 在正确 NUMA node
- page_pool 是 AF_XDP 零拷贝的基础
"""),

("kernel-docs-summary/03-xdp-rings.md", """# 03 — Documentation/networking/xdp-rings-design.rst

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/networking/xdp-rings-design.rst`

## 文档概述

XDP 环形缓冲区设计文档，描述 AF_XDP socket 的 UMEM 和 ring 数据结构。

## 核心内容

### 四个 Ring

| Ring | 生产者 | 消费者 | 作用 |
|------|--------|--------|------|
| FILL | 用户态 | 内核态 | 用户态提供空闲 buffer |
| RX | 内核态 | 用户态 | 内核通知收到的包 |
| TX | 用户态 | 内核态 | 用户态提交待发送包 |
| COMPLETION | 内核态 | 用户态 | 内核通知发送完成 |

### UMEM（共享内存）

- 用户态分配的大块连续内存
- 切分为固定大小的 chunk（通常 4KB）
- 通过偏移量（offset）在 ring 中传递
- 零拷贝模式下 page_pool 的 page 映射到 UMEM

### Ring 同步

- 生产者写 `producer` 指针，消费者读
- 消费者写 `consumer` 指针，生产者读
- 使用 `smp_wmb()` / `smp_rmb()` 保证内存序
- 用户态和内核态共享同一块映射内存

## HFT 要点

- FILL ring 需要预填充足够 buffer，否则收包丢包
- TX + COMPLETION 用于双向零拷贝（行情接收 + 交易发送）
- ring 大小影响丢包率：太小在高 PPS 下溢出
"""),

("kernel-docs-summary/04-af-xdp.md", """# 04 — Documentation/networking/af_xdp.rst

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/networking/af_xdp.rst`

## 文档概述

AF_XDP socket 官方文档，描述用户态零拷贝收发包的完整接口。

## 核心内容

### AF_XDP socket 创建流程

```
1. socket(AF_XDP, SOCK_RAW, 0)
2. 分配 UMEM（mmap 或 malloc + posix_memalign）
3. setsockopt(XDP_UMEM_REG) 注册 UMEM
4. 创建四个 ring（FILL/RX/TX/COMPLETION）
5. setsockopt(XDP_UMEM_FILL_RING / XDP_UMEM_RX_RING / ...)
6. bind(sockfd, sockaddr_xdp{ifindex, queue_id, flags})
7. 预填充 FILL ring（提供空闲 buffer）
8. 轮询 RX ring 收包
```

### 两种模式

| 模式 | flag | 说明 |
|------|------|------|
| Copy | 无 | 内核拷贝包到 UMEM，兼容所有驱动 |
| Zero-copy | XDP_ZEROCOPY | page_pool 直接映射，需驱动支持 |

### 驱动支持情况

| 驱动 | copy | zero-copy |
|------|------|-----------|
| mlx5 | ✅ | ✅ |
| ice | ✅ | ✅ |
| i40e | ✅ | ✅ |
| stmmac | ✅ | ✅（5.x+） |
| virtio-net | ✅ | ✅ |

### 统计信息

```bash
# 查看 AF_XDP socket 统计
xdp-stat queue 0
```
| 统计项 | 含义 |
|--------|------|
| rx_dropped | FILL ring 空导致丢包 |
| rx_invalid_descs | 无效描述符 |
| tx_invalid_descs | 发送无效描述符 |

## HFT 要点

- 零拷贝模式是 HFT 用 AF_XDP 的唯一理由（copy 模式不如直接 recvmsg）
- 绑定到特定 RX 队列：`ethtool -L eth0 combined N` 设置队列数
- FILL ring 预填充数量需 > ring 大小 / 2，避免空 buffer 丢包
"""),

("kernel-docs-summary/05-bpf.md", """# 05 — Documentation/bpf/

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
"""),

("kernel-docs-summary/06-filter.md", """# 06 — Documentation/networking/filter.rst

> **对应 Rosen:** Ch9（Netfilter）/ Ch1（socket filter）
> **内核源码路径:** `Documentation/networking/filter.rst`

## 文档概述

Linux 包过滤器文档，从经典 BPF（cBPF）到 eBPF 的演进。

## 核心内容

### cBPF（Classic BPF）

- 原始 BSD 包过滤器，用于 tcpdump / SO_ATTACH_FILTER
- 指令集简单（load/store/jump/ret）
- 32 位寄存器

### eBPF（Extended BPF）

- 10 个 64 位寄存器
- JIT 编译为原生指令
- 可调用 helper 函数
- verifier 保证安全性

### cBPF → eBPF 转换

内核内部将 cBPF 程序翻译为 eBPF 再执行：
```c
// tcpdump 编译为 cBPF
// 内核自动翻译为 eBPF
// JIT 编译为原生指令
```

### Socket Filter

```c
// 经典 cBPF socket filter
struct sock_filter code[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 12),   // 加载 ethertype
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0800, 0, 1),  // IPv4?
    BPF_STMT(BPF_RET | BPF_K, 65535),  // 接受
    BPF_STMT(BPF_RET | BPF_K, 0),      // 拒绝
};
struct sock_fprog prog = { .len = 4, .filter = code };
setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog));
```

## HFT 要点

- socket filter 可在 recvmsg 之前过滤无关包
- 现代 HFT 更多用 XDP-BPF 替代 socket filter（更早过滤）
- tcpdump 使用 cBPF，在协议栈之后
"""),

("kernel-docs-summary/07-nf-flowtable.md", """# 07 — Documentation/networking/nf_flowtable.rst

> **对应 Rosen:** Ch9（Netfilter）
> **内核源码路径:** `Documentation/networking/nf_flowtable.rst`

## 文档概述

nftables flow table 文档，描述 Netfilter 的硬件流加速机制。

## 核心内容

### Flow Table 是什么

传统 Netfilter 对每个包执行规则匹配，高流量时性能瓶颈。
Flow Table 将已建立连接的流从慢路径（规则匹配）切换到快路径（直接转发）：
- 第一个包走慢路径（完整 Netfilter 规则）
- 建立连接后，流信息缓存到 flow table
- 后续包直接查 flow table，跳过规则匹配

### 工作流程

```
包到达 → 查 flow table
  ├─ 命中 → 快路径（直接转发，O(1)）
  └─ 未命中 → 慢路径（Netfilter 规则匹配）→ 建立 flow entry
```

### 硬件 offload

部分网卡支持将 flow table 卸载到硬件：
- 网卡根据 flow table 直接转发包，不经过 CPU
- 需要网卡驱动支持（mlx5 / ice 等）

## HFT 要点

- HFT 行情流通常不走 Netfilter（已用 XDP 过滤）
- flow table 对非交易流量（管理/监控）的转发加速有用
- 硬件 offload 可减少非交易流量的 CPU 占用
"""),

("kernel-docs-summary/08-txrx.md", """# 08 — Documentation/networking/txrx.rst

> **对应 Rosen:** Ch1（收发包路径）
> **内核源码路径:** `Documentation/networking/txrx.rst`（或相关 driver 文档）

## 文档概述

网卡驱动收发包路径文档，描述 NIC → 内核 → 协议栈的完整数据流。

## 收包路径（RX）

```
1. NIC 收到帧 → DMA 写入 Rx ring 的 buffer（page_pool 分配）
2. NIC 写回 Rx 描述符 → 触发中断
3. 驱动中断处理 → napi_schedule()
4. NAPI 轮询 → napi_poll() → 驱动从 Rx ring 取包
5. 构造 sk_buff（或 XDP 处理）
   ├─ XDP hook（如果有 XDP 程序）
   │   ├─ DROP → page 回收
   │   ├─ PASS → 继续
   │   ├─ REDIRECT → AF_XDP / CPUMAP / DEVMAP
   │   └─ TX → 原路返回
   └─ 无 XDP → 分配 sk_buff
6. napi_gro_receive() → GRO 合并
7. netif_receive_skb() → 协议栈
8. IP 层 → TCP/UDP 层 → socket 队列
```

## 发包路径（TX）

```
1. sendmsg() / sendpage() → 协议栈构造 sk_buff
2. qdisc 排队（fq / pfifo_fast）
3. 驱动 dequeue → 映射 DMA → 写入 Tx ring
4. NIC DMA 发送
5. NIC 完成中断 → 驱动清理 Tx ring → 释放 sk_buff
```

## 现代驱动关键点

| 组件 | 传统 | 现代 |
|------|------|------|
| Rx buffer | alloc_page | page_pool |
| XDP | 无 | native XDP hook |
| GRO | 软件 | 软件 + 硬件 offload |
| TSO | 软件 | 硬件 TSO |
| 中断合并 | 无 | ethtool -C 可调 |

## HFT 要点

- 理解完整 RX 路径是延迟优化的基础
- 每一步都可能引入延迟：中断合并、GRO、协议栈处理、socket 唤醒
- XDP 在步骤 5 最早期处理，是减少延迟的关键
"""),

("kernel-docs-summary/09-sock-sk-buff.md", """# 09 — sk_buff 生命周期

> **对应 Rosen:** Ch1/Ch11（sk_buff 是核心数据结构）
> **内核源码路径:** `Documentation/networking/` 相关 + `include/linux/skbuff.h`

## sk_buff 概述

sk_buff（socket buffer）是 Linux 网络栈的核心数据结构：
- 包含包数据指针 + 元数据（协议头偏移、队列映射、时间戳等）
- 在协议栈各层之间传递
- 分配/释放是网络栈的主要开销之一

## 关键字段

| 字段 | 作用 | HFT 关注 |
|------|------|---------|
| `head` / `data` / `tail` / `end` | 数据缓冲区指针 | 数据访问 |
| `len` / `data_len` | 包长度 | — |
| `protocol` | L3 协议 | 解析 |
| `pkt_type` | 包类型（unicast/multicast/broadcast） | 行情组播 |
| `mark` | skb 标记 | tc-BPF 设置 |
| `priority` | 优先级 | qdisc |
| `queue_mapping` | 网卡队列映射 | 多核收包 |
| `tstamp` / `ktstamp` | 时间戳 | 延迟测量 |
| `cb` | 控制块（每层自定义） | 传递元数据 |
| `skb->users` | 引用计数 | 生命周期 |

## 分配/释放路径

```
alloc_skb() → kmem_cache_alloc(skbuff_head_cache)
  → 分配 sk_buff 结构体（~256 字节）
  → 分配数据缓冲区（__alloc_skb / page_pool）

kfree_skb() → 引用计数减 1
  → 引用计数 = 0 → 释放数据缓冲区 + 释放 sk_buff
```

## XDP 对 sk_buff 的影响

| XDP 动作 | sk_buff | 延迟 |
|----------|---------|------|
| XDP_DROP | 不分配 | 最少 |
| XDP_PASS | 分配（延迟到 XDP 之后） | 减少 |
| XDP_REDIRECT (AF_XDP) | 不分配 | 最少 |
| 无 XDP | 立即分配 | 传统 |

## HFT 要点

- sk_buff 分配开销约 ~100-200 cycles，高 PPS 场景累积显著
- XDP DROP 路径避免 sk_buff 分配，是性能关键
- `skb->tstamp` 可用于测量协议栈各段延迟
- `skb->mark` 配合 tc qdisc 实现流量优先级
"""),

("kernel-docs-summary/10-scaling.md", """# 10 — Documentation/networking/scaling.rst（RPS/RFS/XPS）

> **对应 Rosen:** Ch14（高级主题）
> **内核源码路径:** `Documentation/networking/scaling.rst`

## 文档概述

Linux 网络多核扩展技术文档，描述 RPS/RFS/XPS 三种机制。

## 三种扩展机制

### RPS（Receive Packet Steering）

- 软件层将收到的包分发到不同 CPU 的 backlog 队列
- 基于 4-tuple hash 选择目标 CPU
- 不需要网卡硬件支持
- 代价：跨 CPU 传递 sk_buff 开销

### RFS（Receive Flow Steering）

- RPS 的增强版：将包分发到**正在处理该 socket 的 CPU**
- 保持 CPU cache 亲和性
- `rps_flow_table` 记录 flow → CPU 映射
- 需要网卡支持 aRFS（硬件 RFS）

### XPS（Transmit Packet Steering）

- 发包方向：选择特定 CPU 使用特定 Tx 队列
- 减少锁竞争（每个 Tx 队列有锁）
- 通过 `/sys/class/net/eth0/queues/tx-*/xps_cpus` 配置

## 配置

```bash
# RPS：设置可处理 RX 队列 0 的 CPU
echo f > /sys/class/net/eth0/queues/rx-0/rps_cpus

# RFS：全局 flow table 大小
echo 32768 > /proc/sys/net/core/rps_sock_flow_entries

# XPS：CPU 0-3 使用 Tx 队列 0
echo f > /sys/class/net/eth0/queues/tx-0/xps_cpus
```

## vs XDP CPUMAP

| 维度 | RPS/RFS | XDP CPUMAP |
|------|---------|------------|
| 分发时机 | sk_buff 之后 | sk_buff 之前 |
| CPU 开销 | 分配 sk_buff + IPI | 轻量 |
| 亲和性 | RFS 自动 | BPF 自定义 |
| 灵活性 | hash 固定 | BPF 任意逻辑 |

## HFT 要点

- HFT 行情接收：用 XDP CPUMAP 或网卡 RSS（硬件 hash）替代 RPS
- HFT 交易发送：XPS 配置确保发包在交易 CPU 上
- RFS 对 HFT 意义不大（HFT 通常用 AF_XDP/DPDK 绕过协议栈）
"""),

("kernel-docs-summary/11-msg-zerocopy.md", """# 11 — Documentation/networking/msg_zerocopy.rst

> **对应 Rosen:** Ch11（sendmsg 拷贝模式）
> **内核源码路径:** `Documentation/networking/msg_zerocopy.rst`

## 文档概述

MSG_ZEROCOPY 官方文档，描述零拷贝发送机制和通知接口。

## 核心内容

### 启用零拷贝

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt));
// 或在 sendmsg flags 中传入 MSG_ZEROCOPY
sendmsg(sockfd, &msg, MSG_ZEROCOPY);
```

### 通知机制

零拷贝发送后，buffer 不能立即释放。内核通过 errqueue 通知完成：

```c
// 接收完成通知
struct msghdr msg = {};
char control[CMSG_SPACE(sizeof(struct sock_extended_err))];
msg.msg_control = control;
msg.msg_controllen = sizeof(control);
recvmsg(sockfd, &msg, MSG_ERRQUEUE);

// 解析完成范围
struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
struct sock_extended_err *serr = (void *)CMSG_DATA(cm);
// serr->ee_data = 完成的范围起始序号
// serr->ee_info = 完成的范围结束序号
```

### 适用条件

| 条件 | 要求 |
|------|------|
| 网卡 | 支持 checksum offload + scatter-gather |
| 协议 | TCP / UDP（4.14+ TCP，5.x UDP） |
| 数据量 | > 4KB 才有收益（小包通知开销 > 拷贝开销） |

### 性能数据

| 数据量 | 拷贝模式 | 零拷贝 | 收益 |
|--------|---------|--------|------|
| 1 KB | 1.0 μs | 1.5 μs | -50%（更慢） |
| 4 KB | 1.5 μs | 1.2 μs | +20% |
| 64 KB | 5.0 μs | 1.5 μs | +230% |
| 1 MB | 80 μs | 3.0 μs | +2500% |

## HFT 要点

- 交易报文（< 1KB）：不用零拷贝（通知开销 > 拷贝开销）
- 行情转发（大包）：零拷贝收益大
- io_uring SEND_ZC（6.0+）是更好的零拷贝发送方案（异步 + 批量通知）
"""),

("kernel-docs-summary/12-io-uring-net.md", """# 12 — io_uring 网络

> **对应 Rosen:** 无
> **内核源码路径:** `Documentation/io_uring/`（部分网络操作分散在各子系统文档）

## 文档概述

io_uring 网络操作相关文档，涵盖异步 accept/recv/send/connect 等。

## 核心内容

### 网络相关 opcode

| Opcode | 内核版本 | 对应系统调用 |
|--------|---------|------------|
| IORING_OP_ACCEPT | 5.5 | accept4() |
| IORING_OP_RECV | 5.6 | recv()/recvfrom() |
| IORING_OP_RECVMSG | 5.6 | recvmsg() |
| IORING_OP_SEND | 5.6 | send()/sendto() |
| IORING_OP_SENDMSG | 5.6 | sendmsg() |
| IORING_OP_CONNECT | 5.6 | connect() |
| IORING_OP_SEND_ZC | 6.0 | sendmsg 零拷贝 |

### SQPOLL 模式

```c
struct io_uring_params params = {
    .flags = IORING_SETUP_SQPOLL,
    .sq_thread_idle = 10000,  // 空闲 10 秒后退出
};
io_uring_setup(ENTRIES, &params);
```
- 内核创建 sqpoll 线程持续轮询 SQ ring
- 用户态不需要 `io_uring_enter()`，零系统调用
- 适合低延迟场景（但消耗一个 CPU）

### registered buffers

```c
struct iovec iovecs[N] = { ... };
io_uring_register_buffers(ring, iovecs, N);
```
- 预注册用户态 buffer，内核 pin 住 page
- 后续 recv/send 直接使用注册 buffer
- 避免 `get_user_pages()` 开销

### multishot accept（5.19+）

```c
io_uring_prep_multishot_accept(sqe, sockfd, addr, addrlen, flags);
```
- 一次提交，多次完成
- 每次新连接自动生成 CQE
- 减少 accept 的 SQE 提交开销

## HFT 要点

- SQPOLL + registered buffers = 最低延迟网络 IO
- SEND_ZC = 异步零拷贝发送（优于 MSG_ZEROCOPY 的同步通知）
- multishot accept 适合多连接行情源
"""),
]

# ============================================================
# Part 3: Bootlin Materials (9 files)
# ============================================================

BOOTLIN = [
("bootlin-material/01-architecture.md", """# 01 — 网络栈架构

> **Bootlin 课程模块：** Network Stack Architecture
> **对应 Rosen:** Ch1

## 课程内容

### Linux 网络栈全景

```
用户态:  application → socket API
           ↓
内核态:  socket layer → TCP/UDP → IP → routing → tc → driver → NIC
           ↑                                                    ↓
           └──────────── XDP hook (最早点) ←─────────────────────┘
```

### 核心数据结构

| 结构 | 作用 | 现代变化 |
|------|------|---------|
| `struct net_device` | 网卡抽象 | 新增 XDP 相关字段 |
| `struct sk_buff` | 包数据 + 元数据 | 逐步与 xdp_buff 分流 |
| `struct xdp_buff` | XDP 路径包数据 | 轻量，sk_buff 之前 |
| `struct sock` | socket 抽象 | 新增 sk_reuseport / BPF |
| `struct net` | 网络命名空间 | 容器网络隔离 |

### 关键子系统

| 子系统 | 作用 | 对应 Rosen |
|--------|------|-----------|
| socket layer | 用户态接口 | Ch11 |
| 协议栈 | TCP/UDP/IP | Ch4/Ch11 |
| 路由 | FIB 查找 | Ch5/Ch6 |
| Netfilter/nftables | 包过滤 | Ch9 |
| Traffic Control | QoS | Ch6 |
| XDP | 早数据路径 | 无 |
| NAPI | 收包轮询 | Ch1/Ch14 |

## HFT 要点

- 理解完整数据路径是延迟优化的前提
- XDP 是最早 hook 点，之后是 tc ingress，之后是协议栈
- 每经过一层，延迟增加、灵活性增加
"""),

("bootlin-material/02-rx-path.md", """# 02 — 收包路径

> **Bootlin 课程模块：** RX Path
> **对应 Rosen:** Ch1/Ch11

## 现代 RX 路径（5.x/6.x）

```
1. NIC 收帧 → DMA 写入 Rx ring buffer（page_pool 分配）
2. NIC 更新 Rx 描述符 → 中断或 NAPI 唤醒
3. NAPI poll → 驱动从 Rx ring 取帧
4. XDP hook 执行（如果挂载了 BPF 程序）
   ├─ DROP → page 回收到 page_pool（不分配 sk_buff）
   ├─ PASS → 继续
   ├─ REDIRECT → AF_XDP socket / CPUMAP / DEVMAP
   └─ TX → 修改 MAC 后原路返回
5. 分配 sk_buff（仅 XDP PASS）
6. napi_gro_receive() → GRO 合并
7. netif_receive_skb() → 协议栈
8. IP 层 → 路由查找 → TCP/UDP
9. socket 接收队列 → 唤醒用户进程
```

## 延迟分解（典型）

| 阶段 | 延迟（ns） | 优化手段 |
|------|-----------|---------|
| NIC DMA → 中断 | 100-500 | 中断合并参数 |
| NAPI 调度 → poll | 500-2000 | threaded NAPI |
| XDP 程序 | 10-50 | — |
| sk_buff 分配 | 100-200 | XDP DROP 避免 |
| GRO | 100-500 | 关闭 GRO 减少延迟 |
| 协议栈处理 | 500-2000 | — |
| socket 唤醒 | 1000-5000 | busy polling |
| **总计** | ~2-10 μs | XDP + busy poll 可降到 < 2μs |

## 优化手段

| 手段 | 减少延迟 | 代价 |
|------|---------|------|
| 关闭 GRO | -0.5 μs | 吞吐量降低 |
| SO_BUSY_POLL | -3 μs | CPU 100% |
| XDP 早过滤 | -0.5 μs | BPF 开发 |
| AF_XDP 零拷贝 | -5 μs | 独占 RX 队列 |
| 关闭中断合并 | -0.5 μs | CPU 中断增加 |
"""),

("bootlin-material/03-tx-path.md", """# 03 — 发包路径

> **Bootlin 课程模块：** TX Path
> **对应 Rosen:** Ch11

## 现代 TX 路径（5.x/6.x）

```
1. sendmsg() → 协议栈构造 sk_buff
   ├─ MSG_ZEROCOPY → 不拷贝数据，映射用户 page
   └─ 普通模式 → copy_from_user() 拷贝数据
2. TCP/UDP 处理 → 设置序列号/校验和
3. IP 层 → 路由查找 → 设置 IP 头
4. tc egress → qdisc 排队
   ├─ tc-BPF → 分类/标记
   └─ fq/codel → pacing/调度
5. 驱动 dequeue → DMA 映射 → 写入 Tx ring
6. NIC DMA 发送 → 线缆
7. NIC 完成中断 → 驱动清理 Tx ring → 释放 sk_buff
```

## 发包延迟优化

| 优化 | 效果 | 代价 |
|------|------|------|
| 关闭 TSO | 每包独立发送，降低延迟 | 吞吐量降低 |
| 关闭 qdisc | 减少 qdisc 排队延迟 | 无 QoS |
| MSG_ZEROCOPY | 大包减少拷贝 | 小包通知开销大 |
| io_uring SEND_ZC | 异步零拷贝 | 需要 6.0+ |
| BQL（Byte Queue Limits） | 限制驱动队列长度 | 默认开启 |

## HFT 发包延迟分解

| 阶段 | 延迟（ns） |
|------|-----------|
| sendmsg → sk_buff | 500-2000 |
| 协议栈处理 | 500-1000 |
| qdisc 排队 | 100-10000（取决于队列长度） |
| 驱动 dequeue → DMA | 100-500 |
| NIC 发送 | 100-500 |
| **总计** | ~1-5 μs |
"""),

("bootlin-material/04-xdp.md", """# 04 — XDP 实操

> **Bootlin 课程模块：** XDP
> **对应 Rosen:** 无

## XDP 工具链

### xdp-tools

```bash
# 安装
apt install xdp-tools

# 加载 XDP 程序
xdp-loader load eth0 xdp_program.o

# 查看已加载程序
xdp-loader status

# 卸载
xdp-loader unload eth0 <id>
```

### libbpf + BPF CO-RE

```bash
# 编译
clang -target bpf -O2 -g -c xdp_prog.c -o xdp_prog.o

# 加载（通过 bpftool）
bpftool prog load xdp_prog.o /sys/fs/bpf/xdp_prog
bpftool net attach xdpgeneric name xdp_prog dev eth0
```

## 实验环境

### 树莓派 5 + veth

```bash
# 创建 veth 对
ip link add veth0 type veth peer name veth1
ip link set veth0 up
ip link set veth1 up

# 在 veth0 上加载 XDP（generic 模式）
xdp-loader load --mode generic veth0 xdp_prog.o

# 从 veth1 发包测试
ping -I veth1 10.0.0.1
```

### 实验清单

| 实验 | 目标 |
|------|------|
| XDP DROP all | 验证 XDP 生效（ping 不通） |
| XDP 按端口过滤 | 只放行特定端口 |
| XDP redirect CPUMAP | 多核收包分发 |
| AF_XDP 收包 | 用户态零拷贝接收 |
| XDP + page_pool | 观察 page_pool 统计 |
"""),

("bootlin-material/05-ebpf-net.md", """# 05 — eBPF 网络

> **Bootlin 课程模块：** eBPF Networking
> **对应 Rosen:** 无

## eBPF 网络程序全景

| 类型 | 挂载点 | 工具 |
|------|--------|------|
| XDP | 驱动层 | xdp-loader |
| tc-BPF | tc ingress/egress | tc |
| cgroup-BPF | cgroup | bpftool cgroup |
| sk_msg | socket | skmsg |
| sk_reuseport | SO_REUSEPORT | — |

## tc-BPF 实验

```bash
# 加载 tc-BPF 分类器
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj tc_prog.o sec ingress

# 查看统计
tc filter show dev eth0 ingress

# 删除
tc qdisc del dev eth0 clsact
```

## BPF Map 实验

```bash
# 查看所有 BPF map
bpftool map show

# 查看 map 内容
bpftool map dump id <map_id>

# 查看加载的程序
bpftool prog show
```

## 调试 BPF 程序

```bash
# BPF trace pipe
cat /sys/kernel/debug/tracing/trace_pipe

# bpf_trace_printk() 输出到 trace pipe
# 在 BPF 程序中：
# bpf_trace_printk("pkt: port=%d\\n", port);
```
"""),

("bootlin-material/06-tc.md", """# 06 — Traffic Control

> **Bootlin 课程模块：** Traffic Control
> **对应 Rosen:** Ch6

## 现代 tc 架构

```
发送方向:     socket → qdisc → class → filter → driver
              ↑          ↑        ↑        ↑
              tc-BPF    fq/codel  u32    cls_bpf

接收方向:     driver → tc ingress → 协议栈
                         ↑
                     tc-BPF / cls_bpf
```

## 常用 qdisc

| qdisc | 用途 | HFT 适用 |
|-------|------|---------|
| pfifo_fast | 先进先出（默认） | 简单够用 |
| fq | Flow pacing（TCP 发送节奏） | 行情转发流 |
| fq_codel | fq + AQM（延迟控制） | 非交易流 |
| tbf | Token Bucket（带宽限制） | 限制非交易流量 |
| etf | Earliest Tx First（时间感知） | HFT 交易流调度 |

## HFT tc 配置示例

```bash
# 交易流走高优先级队列
tc qdisc add dev eth0 root handle 1: prio
tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 \
    match ip dport 8001 0xffff flowid 1:1

# 非交易流带宽限制
tc qdisc add dev eth0 parent 1:3 handle 30: tbf \
    rate 10mbit burst 10kb latency 50ms
```
"""),

("bootlin-material/07-nftables.md", """# 07 — Netfilter/nftables

> **Bootlin 课程模块：** Netfilter/nftables
> **对应 Rosen:** Ch9

## nftables 基本操作

```bash
# 创建表
nft add table inet filter

# 创建链
nft add chain inet filter input '{ type filter hook input priority 0 \; }'

# 添加规则
nft add rule inet filter input iif "lo" accept
nft add rule inet filter input tcp dport 22 accept
nft add rule inet filter input tcp dport 9090 ip saddr 10.0.0.0/24 accept
nft add rule inet filter input counter drop

# 查看规则
nft list ruleset

# 保存/恢复
nft list ruleset > /etc/nftables.conf
nft -f /etc/nftables.conf
```

## HFT 防火墙规则示例

```
# 行情源白名单
nft add set inet filter md_sources '{ type ipv4_addr \; flags interval \; }'
nft add element inet filter md_sources '{ 10.0.1.0/24, 10.0.2.0/24 }'
nft add rule inet filter input udp dport 9090 ip saddr @md_sources accept

# 交易端口保护
nft add set inet filter trade_clients '{ type ipv4_addr \; }'
nft add element inet filter trade_clients '{ 10.0.0.5, 10.0.0.6 }'
nft add rule inet filter input tcp dport 8001 ip saddr @trade_clients accept

# 速率限制（非交易流量）
nft add rule inet filter input icmp limit 10/second accept
```

## iptables → nftables 迁移

```bash
# iptables-nft 兼容层（旧 iptables 命令翻译为 nft 规则）
update-alternatives --set iptables /usr/sbin/iptables-nft

# 或使用 iptables-translate 直接翻译
iptables-translate -A INPUT -p tcp --dport 9090 -j ACCEPT
# 输出: nft add rule inet filter input tcp dport 9090 counter accept
```
"""),

("bootlin-material/08-debugging.md", """# 08 — 网络调试工具

> **Bootlin 课程模块：** Network Debugging Tools
> **对应 Rosen:** 无

## 收包诊断工具链

| 工具 | 作用 | 层级 |
|------|------|------|
| `ethtool -S` | 网卡统计（rx_drops/rx_errors） | 驱动 |
| `ethtool -c` | 中断合并参数 | 驱动 |
| `nstat` | 内核网络统计（snmp） | 协议栈 |
| `ss` | socket 统计 | socket |
| `dropwatch` | 丢包位置（内核函数级） | 全路径 |
| `tcpdump` | 包捕获 | 协议栈 |
| `perf` | 内核函数级性能分析 | 全路径 |

## dropwatch：定位丢包

```bash
# 交互模式
dropwatch -l kas

# 输出示例
1 drops at tcp_rcv_established+0x1a2 (0xffffffff8c2b3c42)
5 drops at __netif_receive_skb_core+0x89 (0xffffffff8c28a1b9)
```

## ethtool：网卡级诊断

```bash
# 统计信息
ethtool -S eth0 | grep -E "rx_dropped|rx_missed|rx_no_dma"

# 中断合并
ethtool -c eth0

# 调整（减少延迟）
ethtool -C eth0 rx-usecs 0 rx-frames 0

# 队列信息
ethtool -l eth0   # 查看队列数
ethtool -L eth0 combined 4  # 设置队列数

# offload 状态
ethtool -k eth0
```

## BPF 追踪网络延迟

```bash
# 追踪收包延迟（NIC → socket）
bpftrace -e 'tracepoint:net:netif_receive_skb { @start[args->skb] = nsecs; }
tracepoint:net:net_dev_queue { @latency[nsecs - @start[args->skb]] = count(); }'

# 追踪 XDP 程序执行
bpftrace -e 'kprobe:xdp_prog_run { @start[tid] = nsecs; }
kretprobe:xdp_prog_run /@start[tid]/ { printf("XDP: %d ns\\n", nsecs - @start[tid]); }'
```

## HFT 延迟诊断流程

```
1. ethtool -S → 网卡有没有丢包/错误
2. dropwatch → 内核哪里丢包
3. nstat → 协议栈层面统计
4. ss -ti → TCP 层面（重传/RTT）
5. bpftrace → 函数级延迟分解
6. perf record → CPU 热点分析
```
"""),

("bootlin-material/09-perf-tuning.md", """# 09 — 性能调优

> **Bootlin 课程模块：** Network Performance Tuning
> **对应 Rosen:** Ch14

## 调优清单

### 网卡参数（ethtool）

```bash
# 关闭 GRO/TSO（降低延迟）
ethtool -K eth0 gro off tso off

# 中断合并调零（最低延迟）
ethtool -C eth0 rx-usecs 0 rx-frames 0 tx-usecs 0 tx-frames 0

# 设置队列数
ethtool -L eth0 combined 4

# RSS（硬件多队列 hash）
ethtool -X eth0 equal 4
```

### 内核参数（sysctl）

```bash
# 网络缓冲区
sysctl -w net.core.rmem_max=33554432
sysctl -w net.core.wmem_max=33554432
sysctl -w net.core.rmem_default=262144
sysctl -w net.core.wmem_default=262144

# backlog
sysctl -w net.core.netdev_max_backlog=10000

# TCP 调优
sysctl -w net.ipv4.tcp_no_metrics_save=1
sysctl -w net.ipv4.tcp_low_latency=1  # (已弃用，参考意义)
```

### CPU 亲和性

```bash
# 网卡中断绑定到特定 CPU
# 查看 IRQ 号
cat /proc/interrupts | grep eth0

# 绑定
echo <cpu_mask> > /proc/irq/<irq>/smp_affinity

# NAPI 线程绑定
taskset -c 2 $(pgrep "napi/eth0")
```

### HFT 综合调优清单

| 项目 | 设置 | 目的 |
|------|------|------|
| GRO | off | 减少合并延迟 |
| TSO | off | 降低发送延迟 |
| 中断合并 | 0 | 最低收包延迟 |
| RPS/RFS | off（用 RSS/XDP 替代） | 避免软件分发开销 |
| SO_BUSY_POLL | 50-100 μs | 跳过中断唤醒 |
| CPU 隔离 | isolcpus + irq affinity | 交易 CPU 独占 |
| NUMA 绑定 | numactl --cpunodebind --membind | 避免跨节点访问 |
| QDisc | pfifo_fast 或 noop | 减少排队延迟 |

### 树莓派 5 特殊注意事项

- 树莓派 5 网卡（BCM2712）支持 XDP generic 模式
- AF_XDP 零拷贝模式需要内核 5.x+ 驱动支持
- 中断亲和性需通过 `/proc/irq/` 手动配置
- 无硬件 TSO/GRO offload（纯软件），性能低于服务器网卡
"""),
]

# ============================================================
# Write all files
# ============================================================

print("=== Generating LWN article notes (20 files) ===")
for path, content in LWN:
    write_file(path, content)

print("\n=== Generating kernel-docs notes (12 files) ===")
for path, content in KDOCS:
    write_file(path, content)

print("\n=== Generating Bootlin notes (9 files) ===")
for path, content in BOOTLIN:
    write_file(path, content)

print(f"\nDone! Total: {len(LWN) + len(KDOCS) + len(BOOTLIN)} files generated.")
