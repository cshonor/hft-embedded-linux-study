# Ch 11 §6 激活与停用交换区

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 6. 激活与停用交换区

### 激活 `sys_swapon()`

相对 **简单**：

```
打开 swap 文件/分区
    读磁盘 superblock / 元数据
    填充 swap_info_struct
    按优先级加入激活列表
```

### 停用 `sys_swapoff()` — **极昂贵**

```
不能丢弃 swap 里仍被 PTE 引用的数据
    → try_to_unuse()
        扫描 **所有进程** 页表
        找引用该 swap 区的 PTE
        **强制 swap in** 回物理内存
    物理内存不够 → swapoff **失败**
```

**HFT：** 生产 **latency 机器** 常 **`swapoff -a`** 或 **不配置 swap** — 避免 **silent swap** 与 **误操作 swapoff 风暴**。

---
