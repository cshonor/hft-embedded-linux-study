## 4. 硬件分页机制

> 分页单元：线性地址 → 物理地址；非法访问 → 缺页异常

---

### 一、常规分页（80x86 默认）

| 组件 | 作用 |
|------|------|
| **页目录（Page Directory）** | 第一级索引 |
| **页表（Page Table）** | 第二级索引 |
| **页框（Page Frame）** | 通常 **4 KB** 一页 |

**两级分页** → 32 位线性地址拆成目录索引 + 表索引 + 页内偏移。

---

### 二、扩展分页（Extended Paging）

- **省略页表层级**，直接用 **4 MB 大页**
- 减少 TLB 压力，适合映射大块连续区域（如内核代码）

---

### 三、PAE（Physical Address Extension）

32 位地址引脚默认只能直接寻址 **4 GB RAM**。PAE：

| 特性 | 说明 |
|------|------|
| **36 位物理地址** | 最多 **64 GB** RAM |
| **三级分页** | 在常规两级之上扩展一层 |

→ Linux 如何在统一框架里适配 → [section-5](./section-5-Linux四级分页.md)

---

### 四、加速地址翻译

| 机制 | 作用 |
|------|------|
| **Hardware Cache** | CPU 缓存最近访问的数据/指令 |
| **TLB（Translation Lookaside Buffer）** | 缓存**页表项**，避免每次访问都 walk 页表 |

TLB  miss 时才慢路径查内存中的页表。Linux 如何刷新 TLB → [section-6](./section-6-内存布局与TLB.md)

---

### 五、缺页异常

访问未映射或权限不对的线性地址 → **Page Fault** → 内核处理（分配物理页、COW、swap 等）

→ 深潜：[Ch 8 内存管理](../../chapter-08-memory-management.md) · [Ch 9 进程地址空间](../../chapter-09-process-address-space.md) · [Ch 17 页回收](../../chapter-17-page-reclaim.md)

### 常见陷阱

1. 把 32 位的两级页表直接套用到 64 位——x86-64 用 4 级或 5 级页表，每级 9 位索引，页表项 8 字节
2. 以为 PTE 中只有物理页帧号——PTE 还包含权限位（R/W, U/S）、状态位（Dirty, Accessed）、缓存属性位（PAT, PCD, PWT）等
3. 混淆 4KB 页和 2MB/1GB 大页的页表层级——大页在 PMD 或 PUD 层就终止 walk，不需要 PTE 层

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** x86-64 四级页表中，虚拟地址 48 位如何分配？

<details><summary>答案</summary>

48 位 = 4×9（页表索引）+ 12（页内偏移）。PGD 9 位 + PUD 9 位 + PMD 9 位 + PTE 9 位 + offset 12 位 = 48 位。每级页表 512 个条目（2^9），每个条目 8 字节，一张页表恰好 4KB（一个页）。

</details>

**Q2.** PTE 的 Present 位 = 0 时，内核怎么知道是「换出到 swap」还是「从未分配」？

<details><summary>答案</summary>

PTE 不存在（全 0）= 从未映射。PTE Present=0 但非零 = 已换出或文件映射未加载，高 24 位（swap entry）编码了 swap 类型和 offset。内核用 `pte_present()` 判断，`pte_to_swp_entry()` 解码 swap 信息。

</details>

**Q3.** HFT 为什么推荐用 2MB 大页（huge page）？

<details><summary>答案</summary>

2MB 大页在 PMD 层终止 page walk，省去 PTE 层的一次内存访问。TLB 中一个 2MB 条目覆盖 512 个 4KB 页，大幅减少 TLB miss。对 HFT 热路径（如订单簿内存），`madvise(MADV_HUGEPAGE)` 或 `mmap(MAP_HUGETLB)` 能显著降低延迟抖动。

</details>

</details>

---

← [3. 分段](./section-3-分段机制.md) · 下一节 [5. Linux 四级分页](./section-5-Linux四级分页.md)
