# Ch 14 §3 本书的核心目的

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 本书的核心目的

Mel Gorman 写本书是为了 **弥合鸿沟**：

```
内存管理理论  ←—— 本书 ——→  Linux VM 真实代码
                    │
            尽量架构无关地讲清机制
```

| 读者收获 | 内容 |
|----------|------|
| **理论部分（Ch 1–14）** | Node/Zone/Page、页表、VMA、Buddy、Slab、回收、Swap、shmem、OOM |
| **Code Commentary（附录 A–M）** | 与正文对应的 **源码走读** |

**本仓库笔记：** 各章 `chapter-*.md` 已按 **HFT 落地**（NUMA、mlock、对象池、延迟）加注；附录正文多 **待走读**。

---
