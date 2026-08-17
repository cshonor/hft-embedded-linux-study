## 3. DPDK 突破性能瓶颈的最佳实践

> 早年 IA 多核被认为 **不适合** 高速包处理 — DPDK 用工程实践 **证伪**

---

### 一、轮询模式 (Poll Mode)

| 传统 | DPDK |
|------|------|
| **网卡中断** → 上下文切换、softirq | **轮询** 收包 — **无中断开销** |
| 每包或批量触发 `do_IRQ()` → `net_rx_action()` | PMD 线程死循环调 `rte_eth_rx_burst()` |

**典型 PMD 主循环骨架：**

```c
/* DPDK PMD 主循环 — 每个 lcore 一个 */
static __rte_always_inline void
pmd_loop(unsigned lcore_id)
{
    uint16_t nb_rx;
    struct rte_mbuf *bufs[BURST_SIZE];

    while (!force_quit) {
        /* 轮询收包 — 不阻塞、不等中断 */
        nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);
        if (nb_rx > 0) {
            /* 批量处理 — 减少 per-packet 开销 */
            for (int i = 0; i < nb_rx; i++) {
                process_packet(bufs[i]);
            }
            /* 批量发送 */
            rte_eth_tx_burst(port_id, queue_id, tx_bufs, nb_tx);
            /* 释放已处理 mbuf */
            rte_pktmbuf_free_bulk(bufs, nb_rx);
        }
    }
}
```

 深潜：[chapter-07 轮询与混合中断](../../chapter-07-nic-performance-optimization/notes/section-2-轮询与混合中断模式.md) · [ULK Ch4 I/O 中断](../../../../16-linux-kernel-deep/chapter-04-interrupts-and-exceptions/notes/section-6-IO中断处理.md)

**代价：** 占满 CPU 核 — 需 **绑核**、`isolcpus`，与 idle 友好性 trade-off。HFT 热路径核通常 **专门分配**，不跑其他任务。

---

### 二、用户态驱动 (User-space Driver)

- 网卡驱动运行在 **用户态** — 直接读写 PCI BAR 寄存器
- **减少** 内核 ↔ 用户 **内存拷贝** — DMA 直接写入用户态可见的内存
- **避免** 频繁 **系统调用** 延迟 — 收发包路径零 `syscall`

即 **PMD（Poll Mode Driver）** 体系 — 数据面完全在用户态闭环。

**VFIO 绑定示例（将网卡从内核解绑到用户态）：**

```bash
# 1. 卸载内核驱动
echo "0000:81:00.0" > /sys/bus/pci/drivers/ixgbe/unbind

# 2. 绑定到 vfio-pci
echo "8086 1583" > /sys/bus/pci/drivers/vfio-pci/new_id
# 或
echo "0000:81:00.0" > /sys/bus/pci/drivers/vfio-pci/bind

# 3. 确认 IOMMU 已开启
# GRUB: intel_iommu=on iommu=pt
```

---

### 三、亲和性与独占

| 做法 | 收益 | 实现 |
|------|------|------|
| **CPU 亲和性绑定** | 线程固定逻辑核 | `rte_lcore_id()` / `pthread_setaffinity_np()` |
| **独占 lcore** | 避免跨核迁移 → **Cache miss** ↓ | 内核参数 `isolcpus=2,3` |
| **超线程关闭** | 避免物理核争用 | BIOS 关 Hyper-Threading（HFT 热路径推荐） |

**DPDK lcore 绑核配置：**

```bash
# EAL 参数：指定使用哪些 lcore
./my_app -l 2,3,4,5 -n 4 -- -p 0x3
# -l 2,3,4,5  → 使用 core 2-5
# -n 4        → 4 个内存通道（影响 mempool 创建）

# 内核启动参数：隔离热路径核
isolcpus=2,3,4,5 nohz_full=2,3,4,5 rcu_nocbs=2,3,4,5
```

 [ULK Ch7 调度与 affinity](../../../../16-linux-kernel-deep/chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md) · [14 HFT 绑核](../../../../14-hft-engineering/chapter-05-os-kernel-tuning/)

---

### 四、降低访存开销

| 技术 | 作用 | 量级 |
|------|------|------|
| **Hugepages（大页）** | ↓ TLB miss — 2MB 页 vs 4KB 页 | TLB miss 从 ~30% 降到 <1% |
| **NUMA 感知** | 内存/网卡 **同节点** 分配 | 跨 NUMA 访存延迟 ~2x（~130ns vs ~70ns） |
| **Intel DDIO** | 网卡 DMA 数据 **直达 L3 Cache** — ↓ 内存带宽压力 | 避免 DMA → DRAM → CPU 的一次额外读 |

**大页配置：**

```bash
# 2MB 大页 — 分配 1024 个（共 2GB）
echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
echo 1024 > /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages

# 挂载
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge

# 1GB 大页（HFT 推荐 — 更少 TLB 条目覆盖更大地址空间）
echo 4 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
```

 [ULK Ch8 ZONE/伙伴](../../../../16-linux-kernel-deep/chapter-08-memory-management/) · [CSAPP Ch6 缓存](../../../../02-computer-systems/chapter-06-memory-hierarchy/)

---

### 五、IA 新硬件：SIMD 与超标量

- **SIMD** — 单指令多数据，批量处理包头/字段
  - SSE4.2：`_mm_crc32_u64()` 算 hash key
  - AVX2：`_mm256_loadu_si256()` 批量加载包头
  - AVX-512（Ice Lake+）：512-bit 宽度，8× uint64 一次处理
- **超标量** — 指令级并行 — 无依赖的指令可同周期发射多条

在 **数据面** 做深度向量化 — 与 [Hennessy SIMD/GPU](../../../../15-computer-architecture/chapter-04-vector-simd-gpu/) 概念呼应。

```c
/* DPDK 中 SIMD 的典型用法 — CRC32 计算 hash key */
#include <rte_hash_crc.h>

/* SSE4.2 硬件加速 CRC — 单周期完成 */
uint32_t hash_key = rte_hash_crc(payload, len, initval);

/* 批量 prefetch — AVX 指令一次 prefetch 多个 cache line */
for (int i = 0; i < nb_rx; i += 8) {
    rte_prefetch0(rte_pktmbuf_mtod(bufs[i+4], void *));
}
```

---

← [2. 硬件平台](./section-2-硬件平台与DPDK定位.md) · 下一节 [4. 方法论](./section-4-底层方法论.md)
