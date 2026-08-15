## 7.6 调优指南

### 调优优先级（Gregg + HFT）

1. **消除不必要分配 / 控制 WSS**（Ch 5 应用层）
2. **NUMA 本地** — `numactl`、绑核与内存同节点（Ch 6）
3. **避免 Swap** — `swappiness`、足够 RAM、mlock 关键页
4. **大页** — 减 TLB miss
5. **分配器 / 脏页回写** — 按场景微调
6. **cgroups 限额** — 容器/多租户；裸机低延迟慎用硬限

### 关键 sysctl

| 参数 | 作用 | HFT 倾向 |
|------|------|----------|
| **`vm.swappiness`** | 0–100，倾向 swap 匿名页 vs 回收 file cache | **1–10**（裸机）；**0** 若保证 RAM 充足且接受 OOM 风险 |
| **`vm.min_free_kbytes`** | 保留最小空闲页 | 防突发 alloc 失败；过大浪费 RAM |
| **`vm.dirty_*`** | 脏页回写阈值 | 避免 burst write 拖慢；行情机日志异步 |
| **`vm.overcommit_memory`** | overcommit 策略 | 生产需理解 — 与 OOM 行为相关 |

**禁用 Swap（低延迟裸机常见）：**

```bash
swapoff -a    # 临时；/etc/fstab 去掉 swap 分区持久化
```

→ [18-HFT ch05](../../../17-hft-engineering/chapter-05-操作系统内核极致调优/)

### 大页（Huge Pages）

| 类型 | 配置 | 用途 |
|------|------|------|
| **Transparent Huge Pages (THP)** | 内核自动合并 4KB→2MB | 方便但 **延迟不可预测** — HFT 常 **禁用或 madvise** |
| **Explicit Huge Pages** | `hugetlbfs` / `mmap(MAP_HUGETLB)` | DPDK、确定性延迟 |

→ [06-linux-mm note-THP](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md) · [15-DPDK EAL](../../../14-dpdk/01-Intro-Book/notes/chapter-01-DPDK架构与EAL/)

### 分配器与 NUMA

```bash
# 换 TCMalloc（benchmark 验证后再上生产）
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc.so.4 ./strategy

# 进程绑到 node 0 的 CPU + 内存
numactl --cpunodebind=0 --membind=0 --preferred=0 ./strategy
```

### cgroups 内存控制

| 控制 | cgroup v2 示例 | 场景 |
|------|----------------|------|
| **硬限** | `memory.max` | 容器配额 |
| **swap 行为** | `memory.swap.max` | 限制 swap 使用 |
| **OOM 策略** | 组内 OOM 优先级 | 多服务混部 |

**HFT 共置：** 关键策略进程 **不要** 与未知内存占用的服务同 cgroup；OOM 杀错进程代价极高。

---


### 常见陷阱

1. swappiness 设 0 就安全——0 仍可能 swap（内核 3.5+ 改为 1），且 OOM 风险更高
2. 大页不 benchmark 就上——大页减 TLB miss 但增内部碎片，HFT 需实测确认
3. cgroup memory.max 用在裸机热路径——throttling 后 OOM kill 可能杀错进程

<details>
<summary>自测题（点击展开）</summary>

1. swappiness=0 是否完全禁用 swap？
   <details><summary>答</summary>不完全——内核 3.5+ swappiness=0 等价于 1（极端内存压力仍 swap），应 swapoff -a 彻底禁</details>
2. 大页的 trade-off 是什么？
   <details><summary>答</summary>减 TLB miss（好）但增内部碎片（2MB 页即使只用 1KB 也占 2MB）——需 benchmark 确认</details>
3. HFT 裸机 cgroup 内存控制的风险？
   <details><summary>答</summary>memory.max throttling 后可能 OOM kill——关键策略进程不应与未知服务同 cgroup</details>

</details>


---

← [本章导读](../README.md)
