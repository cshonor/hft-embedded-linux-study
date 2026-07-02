# Ch 5 §2 发现与初始化 (Initializing)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 2. 发现与初始化 (Initializing)

### 架构相关探测

在 **arch 相关 setup** 阶段，内核探测 **可用物理内存边界**，得到例如：

| 参数 | 含义 |
|------|------|
| **`min_low_pfn`** | 最低可用页框号 |
| **`max_low_pfn`** | **NORMAL 区** 最高可用页框号 |
| **高端内存 PFN 范围** | 32 位 **HIGHMEM** 起止（Ch 2 §4） |

### `init_bootmem_core()`

| 步骤 | 做什么 |
|------|--------|
| 初始化 **`bootmem_data`** |  per-node 启动分配器状态 |
| 计算 **位图大小** | 覆盖该 node 全部页框 |
| **为位图本身分配内存** | 仍用 boot 机制（早期可能 **静态/预留区**） |
| **位图初始化为「全保留」** | 再逐步 **free** 可用物理范围 |

**直觉：** 先 **假设全被占用**，再把 **固知空闲的物理 RAM** 标成可分配 — 与 **memblock reserve/free** 现代逻辑同构。

---
