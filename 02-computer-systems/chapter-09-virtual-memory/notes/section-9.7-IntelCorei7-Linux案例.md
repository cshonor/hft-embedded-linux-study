## 9.7 Intel Core i7 / Linux 案例（9.7.1–9.7.2）

> **Ch9 §9.7** · [章导读](../README.md) · 上节 [§9.6 ←](./section-9.6-地址翻译.md) · 下节 [§9.8 →](./section-9.8-内存映射mmap.md)

---

← [本章导读](../README.md)

---

### 9.7.1 Core i7 页表结构

- **4 级页表：** CR3 → PML4 → PDPT → PD → PT → 物理页
- **每级 9 位索引**（512 项），12 位页偏移 → 48 位 VA
- **PTE 格式：** P（present）、R/W、U/S、WT、NC、A（accessed）、D（dirty）、PFN

### 9.7.2 Linux 页表管理

- **Linux 抽象层：** `pgd` → `pud` → `pmd` → `pte`，适配不同架构
- **TLB 结构（i7 典型）：** L1 dTLB（64 项 4KB + 32 项 2MB）、L2 STLB（统一 1536 项）
- **TLB 刷新：** `invlpg` 单页刷新；`cr3` 写入全刷；PCID 减少上下文切换 TLB 失效

**HFT：** 上下文切换后 TLB 冷 → 首批指令慢；用 CPU 绑定 + 大页减少 TLB miss。

### 常见陷阱
1. **i7 用 4 级页表（48 位 VA），Linux 5.x+ 可选 5 级（57 位 VA）** — 不要假设永远是 4 级，检查 `/proc/cpuinfo` 的 `la57`
2. **TLB 是每核私有的，不是共享的** — 上下文切换可能刷 TLB（无 PCID 时），这是切换后首批访存慢的原因
3. **Linux 页表存在内核空间，用户进程不可直接读** — `/proc/self/pagemap` 需 root 或 `CAP_SYS_ADMIN`

### 自测题

<details>
<summary>Q1: Core i7 的 4 级页表分别叫什么？CR3 寄存器存什么？</summary>

PML4 → PDPT → PD → PT。CR3 存 PML4 表的物理地址（顶层页表基址）。

</details>

<details>
<summary>Q2: x86-64 48 位 VA 如何分配给 4 级页表索引和页偏移？</summary>

12 位页偏移 + 4 × 9 位索引 = 48 位。每级 9 位 = 512 项，每项 8B → 每级页表恰好 4KB（一页）。

</details>

<details>
<summary>Q3: 上下文切换时 TLB 会怎样？PCID 如何帮助？</summary>

无 PCID 时切换需刷 TLB（写 CR3），新进程首批访存全部 TLB miss。PCID 给每进程分配 ID，切换时可不刷 TLB，减少切换开销。

</details>

<details>
<summary>Q4: HFT 中如何减少 TLB miss？</summary>

1) CPU 绑定减少上下文切换；2) 大页（2MB/1GB）减少 TLB 项数；3) `mlock` 防止页被换出；4) 预 fault 热数据区域。

</details>

---

← [§9.6 ←](./section-9.6-地址翻译.md) · [本章导读](../README.md) · [§9.8 →](./section-9.8-内存映射mmap.md)
