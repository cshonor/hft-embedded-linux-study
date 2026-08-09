## 2. PFRA 与页分类 (Page Frame Reclaiming Algorithm)

> **PFRA** — 谁在什么时候 **释放哪些页框**

---

### 一、四类页

| 类型 | 示例 | 回收方式 |
|------|------|----------|
| **不可回收** | 空闲页、保留页、kmalloc、内核栈、**mlock** 页 | **不回收** |
| **可交换 (Swappable)** | 用户 **匿名页**、tmpfs、IPC 共享内存 | 换出到 **swap** |
| **可同步 (Syncable)** | 文件映射页、**页缓存**、块缓冲 | 脏页 **写回** 后丢弃 |
| **可丢弃 (Discardable)** | 未使用页、分配器 **缓存** | **直接** 回收 |

→ 页缓存：[Ch 15](../chapter-15-page-cache/) · 匿名页：[Ch 9](../chapter-09-process-address-space/)

---

### 二、PFRA 设计原则

1. **先无害** — 优先 **未被进程引用** 的磁盘/内存 **缓存**  
2. **用户页可收** — 除非 **`mlock`** 等锁定  
3. **共享页** — 回收时必须 **清除所有引用该页的 PTE**（→ [section-3](./section-3-反向映射.md)）  
4. **只收 unused** — 通过 LRU 判断「久未访问」（→ [section-4](./section-4-LRU链表.md)）  

---

### 三、与伙伴系统的关系

分配失败 → PFRA **腾页** → 伙伴系统再分配。

→ [Ch 8 伙伴系统](../chapter-08-memory-management/notes/section-2-页框管理.md)

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 反向映射](./section-3-反向映射.md)
