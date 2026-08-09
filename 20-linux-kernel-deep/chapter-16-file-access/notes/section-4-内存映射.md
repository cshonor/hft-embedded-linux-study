## 4. 内存映射 (Memory Mapping)

> 文件内容出现在 **进程地址空间** — 仍常共享 **页缓存**

---

### 一、`mmap()` 做什么

建立 **文件 offset ↔ 虚拟地址** 的 VMA — **不立即** 读盘、**不立即** 分配物理页。

→ VMA：[Ch 9 section-3](../chapter-09-process-address-space/notes/section-3-内存区VMA.md)

---

### 二、请求调页 (Demand Paging)

进程 **首次 touch** 映射地址 → **缺页异常**：

```
缺页
    ↓
do_page_fault → 文件映射路径
    ↓
FS nopage（通常 filemap_nopage()）
    ↓
从磁盘读入 **页缓存**（若尚无）
    ↓
更新进程页表 → 指向该页框
```

**与 read 共享页缓存** — 另一进程 `read` 同一偏移可 **命中** 已 mmap 进来的页。

→ 缺页框架：[Ch 9 section-4](../chapter-09-process-address-space/notes/section-4-缺页异常.md)

---

### 三、非线性内存映射 (Non-Linear Mappings)

Linux 2.6 **`remap_file_pages()`**：

- 将文件中 **离散页** 映射到 **连续** 虚拟区间  
- 避免对大文件 **多次 mmap** → VMA **碎片化**  

适用：大文件 **稀疏** 访问模式。

---

### 四、mmap vs read

| | read/write | mmap |
|--|------------|------|
| 接口 | 反复 syscall + 拷贝 | touch 映射区（缺页时拷贝/映射） |
| 缓存 | 页缓存 | 通常 **同一页缓存** |
| HFT | 小文件、流式 | 大只读数据集、共享内存式访问 |

---

← [3. 读写与预读](./section-3-读写与预读.md) · 下一节 [5. AIO](./section-5-异步IO.md)
