## 9.3 虚拟内存作为缓存工具

> **Ch9 §9.3** · [章导读](../README.md) · 上节 [§9.2 ←](./section-9.2-地址空间.md) · 下节 [§9.4 →](./section-9.4-虚拟内存作为内存管理工具.md)
> ↔ [Harris §8.4 虚拟存储器](../../../00-digital-logic-cpu/ch08_memory/8.4_虚拟存储器.md)
> ↔ [Hennessy §2.4 虚拟内存](../../../19-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.4-虚拟内存与虚拟机.md)

---

← [本章导读](../README.md)

---

### 虚拟内存 = DRAM 缓存磁盘

- **概念：** VM 将 DRAM 视为磁盘（swap）的缓存，以 **页（4KB）** 为单位
- **页表（PTE）= 缓存标记** — 记录页是否驻留（valid bit）、位置、权限
- **命中（page hit）** — 页在 DRAM，直接访问
- **缺页（page fault）** — 页不在 DRAM，OS 从磁盘装入：
  1. 触发异常 → OS 接管
  2. 选牺牲页（LRU/近似）→ 若脏则写回磁盘
  3. 从磁盘读入新页 → 更新 PTE
  4. 重启触发指令

| 概念 | VM 缓存 | CPU cache |
|------|---------|-----------|
| 缓存粒度 | 页 4KB | 行 64B |
| 缓存介质 | DRAM 缓存磁盘 | SRAM 缓存 DRAM |
| miss 代价 | μs 级 | ns 级 |
| 替换策略 | OS 软件 LRU | 硬件近似 LRU |

**局部性保证可行性：** 工作集 ≪ VA 空间，多数页无需驻留。

### 常见陷阱
1. **VM 缓存粒度是页（4KB），CPU cache 粒度是行（64B）** — 不要混淆两个层次的缓存
2. **缺页代价远大于 cache miss** — 缺页可能触发磁盘 I/O（μs 级），cache miss 只等 SRAM（ns 级）
3. **demand paging ≠ prefetch** — 默认按需加载，不预读；OS 可预读相邻页但不是必须

### 自测题

<details>
<summary>Q1: 虚拟内存作为缓存工具时，缓的是什么？被缓的是什么？</summary>

DRAM 缓存磁盘（swap 空间）。热页在 DRAM，冷页在磁盘，以 4KB 页为传输单位。

</details>

<details>
<summary>Q2: page hit 和 page fault 分别是什么？</summary>

page hit = 访问的页已在 DRAM，PTE valid bit=1。page fault = 页不在 DRAM，触发异常由 OS 从磁盘装入。

</details>

<details>
<summary>Q3: 为什么虚拟内存方案可行？基础前提是什么？</summary>

局部性原理（spatial + temporal）。程序工作集远小于 VA 空间，大部分页不需要同时驻留。

</details>

<details>
<summary>Q4: 缺页处理时 OS 做哪些步骤？</summary>

1) 触发异常；2) 选牺牲页（若脏先写回）；3) 从磁盘读入新页；4) 更新 PTE；5) 重启触发指令。

</details>

---

← [§9.2 ←](./section-9.2-地址空间.md) · [本章导读](../README.md) · [§9.4 →](./section-9.4-虚拟内存作为内存管理工具.md)
