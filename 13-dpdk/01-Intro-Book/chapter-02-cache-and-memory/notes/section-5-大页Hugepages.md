## 5. 突破 TLB 瓶颈：大页 (Hugepages)

> **4KB 小页** → 页表项爆炸 → **TLB miss** 频发

---

### 一、问题

大内存应用（DPDK mbuf 池、大 ring）若用 **4KB 页**：

- 需要 **海量 PTE** — 2GB 内存 ÷ 4KB = **524,288 个页表项**
- **TLB 装不下**（L1 dTLB ~72 条目）→ 频繁 miss → 遍历 **4 级页表**

| 页大小 | 2GB 所需 PTE | TLB 覆盖 (72 条目) |
|--------|:---:|:---:|
| 4 KB | 524,288 | 288 KB |
| 2 MB | 1,024 | 144 MB |
| 1 GB | 2 | 72 GB |

 TLB：[section-2](./section-2-阶梯式Cache系统.md) · [ULK Ch2](../../../../16-linux-kernel-deep/chapter-02-memory-addressing/)

---

### 二、DPDK 的大页

| 页大小 | 典型用途 | GRUB 参数 |
|--------|----------|-----------|
| **2MB** | 常用 hugepage — 够用且灵活 | `hugepagesz=2M hugepages=1024` |
| **1GB** | 更大池、减 TLB 压力 — HFT 首选 | `hugepagesz=1G hugepages=4` |

**效果：** 同样物理内存 → **页表项数量 ÷512**（2MB）→ TLB **命中率高**。

---

### 三、如何使用

**1. 系统配置（NUMA 感知）：**

```bash
# 2MB 大页 — 在每个 NUMA 节点上分别预留
echo 1024 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
echo 1024 > /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages

# 1GB 大页（HFT 推荐）
echo 4 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
echo 4 > /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages

# 挂载 hugetlbfs
mkdir -p /mnt/huge
mount -t hugetlbfs nodev /mnt/huge

# 持久化 — /etc/fstab
nodev /mnt/huge hugetlbfs defaults 0 0
```

**2. EAL 初始化时映射大页：**

```bash
# DPDK EAL 参数 — 指定每个 NUMA 节点的大页内存
./my_app -l 2,3 -n 4 --socket-mem=1024,1024 -- -p 0x3
#                    ↑ node0=1GB, node1=1GB

# 或使用 1GB 大页
./my_app -l 2,3 -n 4 --socket-mem=2048,2048 --huge-worker-stack -- -p 0x3
```

EAL 内部通过 `mmap(MAP_HUGETLB)` 映射大页内存，并将物理地址通过 `/proc/self/pagemap` 查询出来，用于 DMA buffer 注册。

**3. mempool 从大页分配：**

```c
/* rte_pktmbuf_pool_create 内部使用 rte_memzone_reserve */
/* rte_memzone_reserve → rte_malloc → 大页堆 */
struct rte_mempool *pool = rte_pktmbuf_pool_create(
    "MBUF_POOL",
    32768,      /* 32K 个 mbuf */
    256,        /* per-lcore cache size */
    0,
    RTE_MBUF_DEFAULT_BUF_SIZE,  /* 2176B (2048 data + 128 headroom) */
    rte_socket_id()  /* NUMA 感知 — 从本节点大页分配 */
);
```

**HFT 检查清单：**

```bash
# 确认大页分配
grep Huge /proc/meminfo
# HugePages_Total:    2048
# HugePages_Free:     2048
# HugePages_Rsvd:        0
# HugePages_Surp:        0
# Hugepagesize:       2048 kB

# 确认 NUMA 分布
cat /sys/devices/system/node/node*/hugepages/hugepages-2048kB/nr_hugepages
# 1024
# 1024

# 确认应用已映射大页
cat /proc/$(pidof my_app)/smaps | grep -i huge
```

 EAL：[Ch1 HelloWorld](../../chapter-01-dpdk-intro/notes/section-6-编程实例入门.md) · mbuf：[Ch6 §6](../../chapter-06-pcie-packet-io/notes/section-6-Mbuf与Mempool.md)

---

← [4. 一致性](./section-4-Cache一致性与无锁设计.md) · 下一节 [6. DDIO/NUMA](./section-6-DDIO与NUMA.md)
