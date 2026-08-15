## 6.1 存储技术（6.1.1–6.1.4）

> ↔ [Hennessy §2.2 存储器技术](../../../18-computer-architecture/chapter-02-memory-hierarchy-design/notes/section-2.2-存储器技术与优化.md)


### 6.1.1 随机访问存储器 (RAM)

| 类型 | 特点 |
|------|------|
| **SRAM** | 快、贵、低功耗/bit — 用于 **cache** |
| **DRAM** | 慢于 SRAM、便宜 — **主存**；需刷新 |
| **VRAM/HBM** | 高带宽变体 — GPU/部分服务器 |

- **访问时间：** DRAM ~50–100ns 量级；L1 ~1ns 量级（见原书表格）
- **行缓冲 (row buffer)** — 同 row 命中更快（类似「DRAM 内 cache」）

### 6.1.2 磁盘存储

- 机械硬盘：**寻道 + 旋转延迟 + 传输** — 毫秒级
- **顺序读** 远快于随机小 I/O

### 6.1.3 固态硬盘 (SSD)

- **Flash** — 无机械部件；随机读好，**写放大**、擦除块
- **NVMe** — PCIe  attached，微秒–毫秒；仍比 DRAM 慢数量级

### 6.1.4 存储技术趋势

- **CPU–内存差距 (memory wall)** 持续扩大 → cache 层次更深
- **价格/容量/速度** 三角 — 层次结构不会消失

**HFT：**

- 热路径数据 **驻留 DRAM + L3**；日志/回放 **顺序写 NVMe**
- 共置机器 **足够 DRAM** 装 working set；swap 禁用（→ [18-HFT](../../../17-hft-engineering/)）
- DPDK **mbuf 池** 预分配 — 避免 tick 上 malloc（→ [15-DPDK](../../../14-dpdk/)）

---

### 常见陷阱

1. **以为 DRAM 和 cache 差不多快** — DRAM ~50-100ns，L1 ~1ns，差 50-100 倍。一次 DRAM miss 能让 HFT 热路径延迟暴涨上百纳秒。
2. **热路径数据不在 DRAM 就放心了** — DRAM miss 到 DRAM 仍有 ~100ns；HFT 热数据要驻留 L1/L2/L3，不只是「在内存里」。
3. **swap 没禁用** — 热路径数据被换出到磁盘，一次 page fault 就是毫秒级。HFT 服务器必须 `swapoff` + `mlock` 关键内存。

### 自测题

<details>
<summary>1. SRAM 和 DRAM 的主要区别是什么？各用在哪里？</summary>

**SRAM**：快（~1ns）、贵、低功耗/bit，用于 **cache**（L1/L2/L3）。**DRAM**：慢（~50-100ns）、便宜、需定期刷新，用于 **主存**。HFT 热数据要尽量留在 SRAM（cache）层。
</details>

<details>
<summary>2. memory wall 是什么？为什么 cache 层次越来越深？</summary>

**CPU 速度增长远快于 DRAM 速度增长**，差距（memory wall）持续扩大。为了弥合差距，CPU 增加更多 cache 层级（L1→L2→L3），让热数据留在离核心更近的 SRAM 中。层次结构不会消失。
</details>

<details>
<summary>3. HFT 服务器为什么必须禁用 swap？</summary>

swap 会把内存页换出到磁盘。一旦热路径数据被换出，访问触发 **page fault**，延迟从纳秒暴涨到**毫秒**（慢 10⁶ 倍）。HFT 服务器必须 `swapoff` + `mlock` 锁定关键内存 + 足够 DRAM 装 working set。
</details>

---

← [本章导读](../README.md)
