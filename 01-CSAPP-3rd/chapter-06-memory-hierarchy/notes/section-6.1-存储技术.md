## 6.1 存储技术（6.1.1–6.1.4）

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
- 共置机器 **足够 DRAM** 装 working set；swap 禁用（→ [12-HFT](../../../15-HFT-Low-Latency-Practice/)）
- DPDK **mbuf 池** 预分配 — 避免 tick 上 malloc（→ [12-DPDK](../../../12-DPDK-Low-Latency-Network/)）

---

← [本章导读](../README.md)
