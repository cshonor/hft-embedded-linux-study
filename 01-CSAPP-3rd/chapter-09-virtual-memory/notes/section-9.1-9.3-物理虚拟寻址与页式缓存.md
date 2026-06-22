## 9.1–9.3 物理/虚拟寻址与页式缓存

### 9.1 物理和虚拟寻址

- **物理地址 (PA)** — 实际 DRAM 字地址
- **虚拟地址 (VA)** — 进程使用的地址；**MMU** 翻译 VA→PA

### 9.2 地址空间

- 每进程 **独立虚拟地址空间** — 同 VA 不同进程映射不同 PA
- x86-64 用户空间典型 **48 位** 有效（256TB 量级概念）

### 9.3 虚拟内存作为缓存工具

**DRAM 是磁盘的 cache** — 数据组织成 **页 (page)**，通常 **4KB**（另有 2M/1G 大页）

#### 9.3.1–9.3.2 DRAM 缓存与页表

- **页表** — 每进程（逻辑上）VA page → PA page 或「不在内存」
- **页表项 (PTE)** — 有效位、权限、物理页号

#### 9.3.3 页命中

- PTE 有效且页在 DRAM — **无异常**，MMU 直接翻译

#### 9.3.4 缺页 (Page Fault)

- PTE 无效或权限违例 → **故障** 进内核：
  - ** demand paging** — 分配物理页，从磁盘读入（或零页）
  - **swap out/in** — 换页

**HFT：** tick 上 **major/minor fault** 都是抖动来源 — `perf stat -e page-faults,major-faults`

#### 9.3.5–9.3.6 分配页面与局部性

- 内核 **按需** 给页；**工作集** 应 fit DRAM
- **局部性** 降 fault rate — 顺序访问、紧凑工作集

→ [Ch 6 局部性](../chapter-06-memory-hierarchy/notes/section-6.2-局部性.md) · [Ch 8 故障](../chapter-08-exceptional-control-flow/notes/section-8.1-异常.md)

---

← [本章导读](../README.md)
