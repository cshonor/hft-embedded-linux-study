## 9.6 地址翻译

> **Ch9 §9.6** · [章导读](../README.md) · 上节 [§9.5 ←](./section-9.5-虚拟内存作为保护工具.md) · 下节 [§9.7 →](./section-9.7-IntelCorei7-Linux案例.md)
> ↔ [Harris §8.4 虚拟存储器](../../../00-digital-logic-cpu/ch08_memory/8.4_虚拟存储器.md)

---

**VA 划分（概念）：** `VPN | VPO` → 经页表得 `PPN | PPO`

#### 9.6.1 结合 Cache 与 VM

- **物理寻址 cache (PA)** vs **虚拟寻址 cache (VA)** — 现代 x86 常用 **物理索引物理标记 (PIPT)** L2/L3，避免别名
- **缺页或权限检查** 在 cache 访问路径上

#### 9.6.2 TLB

- **页表在内存** — 每次翻译读 PTE 太慢
- **TLB** — 页表缓存（MMU 内），**全相联/组相联**，典型 64–1024 项
- **TLB miss** → 页表 walk（可能多级）— 数十周期

**大页 (2MB/1GB)：** 同样工作集 **更少 TLB 项** — HFT 关键优化

#### 9.6.3 多级页表

- 单级页表 4KB 页 × 48 位 VA → 表太大
- **四级页表** (x86-64) — 未用区域不占物理页

#### 9.6.4 端到端翻译

```
VA → TLB hit? → PA → L1 → ... → 或 TLB miss → 页表 walk → 可能 page fault
```

---

### 常见陷阱
1. **TLB miss ≠ page fault** — TLB miss 只是缓存未命中，MMU 去 walk 页表找 PTE；page fault 是 PTE 本身标记页不在内存
2. **多级页表省空间但增加访存次数** — 每级页表需一次内存访问，4 级 = 4 次访存（TLB miss 时）；TLB 命中则只需 1 次
3. **大页减 TLB 压力但增内部碎片** — 2MB 大页覆盖更多 VA，但分配后未用部分浪费；HFT 用大页覆盖热数据区域

### 自测题

<details>
<summary>Q1: VA 如何划分为 VPN 和 VPO？4KB 页时各多少位？</summary>

VPO = 页内偏移（4KB → 12 位），VPN = 剩余位。x86-64 48 位有效 VA：VPO 12 位 + VPN 36 位。

</details>

<details>
<summary>Q2: TLB miss 时 MMU 做什么？和 page fault 有何区别？</summary>

TLB miss → MMU walk 页表（多级），从内存读 PTE 装入 TLB，然后重试翻译。page fault 是 PTE 显示页不在 DRAM，需 OS 介入从磁盘装入。TLB miss 是硬件处理，page fault 是软件处理。

</details>

<details>
<summary>Q3: 4 级页表如何节省内存？单级页表有什么问题？</summary>

单级页表：48 位 VA / 4KB 页 = 2^36 个 PTE × 8B ≈ 512GB/进程，不可行。4 级页表只分配用到的区域，未用区域不占物理页。

</details>

<details>
<summary>Q4: HFT 为什么用大页（2MB/1GB）？具体好处是什么？</summary>

同样工作集用更少 TLB 项覆盖。2MB 页 vs 4KB 页 → TLB 项减少 512 倍，减少 TLB miss 导致的页表 walk（每次 walk 数十周期），降低尾延迟。

</details>

---

← [§9.5 ←](./section-9.5-虚拟内存作为保护工具.md) · [本章导读](../README.md) · [§9.7 →](./section-9.7-IntelCorei7-Linux案例.md)
