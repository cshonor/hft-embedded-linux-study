## 7.3 硬件与软件架构

### 硬件：DRAM、UMA、NUMA、MMU、TLB

| 组件 | 性能要点 |
|------|----------|
| **DRAM** | 容量大、延迟远高于 cache |
| **UMA** | 所有 CPU 访存延迟一致（老单机） |
| **NUMA** | 每 socket **本地内存** 快，**远程** 慢 1.5–3× 常见 |
| **MMU** | 虚拟 → 物理地址翻译 |
| **TLB** | 页表项缓存；**TLB miss** 触发页表 walk — 贵 |
| **Huge Pages** | 2MB / 1GB 页 → **同样映射范围 TLB 项更少** |

**NUMA 本地性（HFT 必做）：**

```bash
numactl --hardware          # 看节点与距离
numastat                    # 本地 vs 远程分配
numactl --cpunodebind=0 --membind=0 ./strategy
```

→ Ch 6 [绑核与 NUMA](../../chapter-06-cpus/) · [19-Hennessy](../../../15-computer-architecture/)

### Linux 释放内存机制（由轻到重）

```
① Free List（有空闲页直接用）
    ↓ 不足
② 回收 Page Cache（文件页，受 vm.swappiness 影响倾向）
    ↓ 仍不足
③ kswapd 后台扫描回收
    ↓ 仍不足
④ Direct Reclaim（在 fault/alloc 路径上同步回收 — 拖慢当前线程）
    ↓ 仍不足
⑤ OOM Killer（选进程杀）
```

| 阶段 | HFT 信号 |
|------|----------|
| **Direct reclaim** | 延迟毛刺、BPF `drsnoop` 有事件 |
| **Swap out (so)** | **不可接受** 于 tick 热路径机器 |
| **OOM** | 进程消失 — 比慢更糟 |

→ [06-linux-mm ch13 内存耗尽](../../../06-linux-mm/chapter-13-out-of-memory-management/)

### 内存分配器

**内核：Slab / SLUB**

| 分配器 | 作用 |
|--------|------|
| **Slab / SLUB** | 对象缓存（dentry、inode、task_struct…）— 减少频繁 alloc_page |
| 查看 | `slabtop` — 哪个 cache 涨 |

**用户态：**

| 分配器 | 特点 | HFT |
|--------|------|-----|
| **glibc (ptmalloc/dlmalloc)** | 默认；多线程下 **arena 锁** 可能竞争 | 热路径少 malloc |
| **TCMalloc** | Google；per-thread cache，低锁竞争 | 可 `LD_PRELOAD` 对比 tail latency |
| **jemalloc** | 碎片控制好、arena 可配置 | 长期运行服务常用 |

**原则：** HFT tick 路径 **预分配 / 对象池 / 无分配** 优于换分配器；换分配器是 **第二道防线**。

→ [02-CSAPP Ch9 malloc](../../../02-computer-systems/chapter-09-virtual-memory/) · Ch 5 [GC vs 手动管理](../../chapter-05-applications/)

---


### 常见陷阱

1. 不绑 NUMA——跨 socket 访存延迟 1.5-3 倍，HFT 必须 CPU + 内存同 NUMA 节点
2. THP 当大页用——THP 自动合并但延迟不可预测（khugepaged 后台扫描），HFT 应用显式大页
3. Direct Reclaim 不监控——内存不够时同步回收拖慢当前线程，BPF drsnoop 才能看到

<details>
<summary>自测题（点击展开）</summary>

1. NUMA 对 HFT 的影响？
   <details><summary>答</summary>跨 socket 访存延迟 1.5-3 倍——numactl --cpunodebind=0 --membind=0 绑同节点</details>
2. THP 为什么不适合 HFT？
   <details><summary>答</summary>khugepaged 后台扫描合并 4KB→2MB 延迟不可预测——应用显式大页（mmap MAP_HUGETLB）</details>
3. Direct Reclaim 如何检测？
   <details><summary>答</summary>BPF drsnoop 追踪 direct reclaim 路径延迟——vmstat 只看 si/so 不够</details>

</details>


---

← [本章导读](../README.md)
