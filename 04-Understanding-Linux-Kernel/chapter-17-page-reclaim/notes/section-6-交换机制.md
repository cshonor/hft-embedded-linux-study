## 6. 交换机制 (Swapping)

> 无磁盘映像的页 — **换出到 swap** 才能回收物理页框

---

### 一、哪些页需要 swap

| 页类型 | 无文件 backing | 回收方式 |
|--------|----------------|----------|
| **匿名页** | 是 | **swap out** |
| 私有脏 mmap（匿名类） | 是 | swap |
| **IPC 共享内存** | 视配置 | 可 swap |
| **文件映射 / 页缓存** | 有磁盘映像 | **写回或丢弃** — 不必 swap |

---

### 二、交换区数据结构

| 组件 | 说明 |
|------|------|
| **交换区** | 独立 **分区** 或 **文件** |
| **`swap_info_struct`** | 描述每个 swap 设备 |
| **`swap_map[]`** | 每个 **page slot** 使用计数 / 所有者 |

页换出后，原 PTE 不再指向物理页，改为 **swapped-out 标识符**（swap 索引 + slot 索引）— 缺页时再 **swap in**。

→ 缺页换入：[Ch 9 section-4](../chapter-09-process-address-space/notes/section-4-缺页异常.md)

---

### 三、交换高速缓存 (Swap Cache)

换入/换出的 **中转站**：

- 共享匿名页换出时 **先入 swap cache**  
- 避免：**进程 A 正在写盘换出**，**进程 B 同时缺页换入** → 竞争 / 数据不一致  

同一 swap slot 在 cache 中 **至多一页** 结构体对应。

> **深潜可选：** `add_to_swap_cache`、`try_to_free_swap` 并发路径。

---

### 四、交换令牌 (Swap Token)

防 **swap thrashing**（刚换出又换入 → CPU 空转）：

- 内核维护 **swap token**  
- **持有者** 的页框 **暂豁免** PFRA 回收  

给「正在密集访问」的进程 **喘息窗口**。

HFT：**生产交易主机常禁用 swap 或 mlock** — swap 颠簸 = 毫秒～秒级停顿。

---

### 五、本章小结

```
分配失败 / 水位低
    ↓
PFRA：LRU 尾部选页
    ↓
反向映射 → 清 PTE
    ├─ 页缓存 → 写回（若 dirty）→ 释放
    └─ 匿名页 → swap cache → swap out → 释放
    ↓ 仍不够
OOM Killer
```

---

### 六、后续章节索引

| Ch 17 主题 | 继续读 |
|------------|--------|
| 伙伴 / Slab | [Ch 8 内存管理](../chapter-08-memory-management/) 🔴 |
| 缺页 / swap in | [Ch 9 进程地址空间](../chapter-09-process-address-space/) 🔴 |
| 页缓存回收 | [Ch 15 页缓存](../chapter-15-page-cache/) ⚪ |
| 07 Gorman | [页回收 / rmap](../../../05-Linux-Virtual-Memory-Manager/) |
| Ext2 文件系统 | [Ch 18 Ext2/Ext3](../chapter-18-ext2-ext3/) ⚪ |
| HFT 内存 | [15 HFT 工程](../../../15-HFT-Low-Latency-Practice/) · [03 SysPerf Ch 7](../../../13-Systems-Performance-2nd/chapter-07-memory/) |

---

← [5. OOM](./section-5-执行时机与OOM.md) · 下一章 [Ch 18 Ext2/Ext3](../chapter-18-ext2-ext3/)
