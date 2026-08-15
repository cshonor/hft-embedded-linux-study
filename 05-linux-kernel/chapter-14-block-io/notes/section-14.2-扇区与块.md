## ② 扇区与块 · Sectors and Blocks

| 术语 | 层级 | 典型大小 |
|------|------|----------|
| **扇区（sector）** | **硬件** 最小可寻址单元 | 常见 **512B**（4Kn 盘 4096B） |
| **块（block）** | **文件系统 / 内核** 逻辑最小单元 | 扇区的 **2^n 倍** · ≤ **一页**（512B / 1KB / **4KB**） |

```
磁盘扇区（512B）──► 多个扇区组成 FS「块」（如 4KB）
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 扇区(sector)、块(block)、页(page) 的关系和大小？

<details><summary>答案</summary>

扇区 = 硬件最小传输单元（通常 512B，现代 4K）。块 = 文件系统最小分配单元（通常 4KB = 8 sector）。页 = 内存管理最小单元（4KB）。文件系统块 = 内存页 = 4KB 不是巧合：VFS 设计为块大小 = PAGE_SIZE，使 page cache 和文件系统块一一对应。

</details>

</details>
---
