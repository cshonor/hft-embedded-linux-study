# Ch 9 进程地址空间 · Process Address Space

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 用户态虚拟内存 — `mm_struct`、VMA、缺页、请求调页、COW

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **VMA 红黑树 + 链表** | **maple tree** 取代红黑树（6.1 起） | [The maple tree](https://lwn.net/Articles/845507/) |
| `vm_area_struct` | 仍存在，但查找结构变了 | [A maple tree for VMA tracking](https://lwn.net/Articles/895690/) |
| `find_vma()` | 改为 maple tree 查找 | [Maple tree documentation](https://docs.kernel.org/core-api/maple_tree.html) |
| 缺页处理路径 | 概念不变，但 `fault` 回调接口更新 | [Kernel doc: mm](https://docs.kernel.org/admin-guide/mm/) |

> **原则**：VMA 管理从红黑树到 maple tree 是数据结构层面的重构。`task_struct→mm_struct→VMA` 的层次不变，但查找路径完全不同。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 内存描述符 | [notes/section-2-内存描述符.md](./notes/section-2-内存描述符.md) |
| 3. 内存区 VMA | [notes/section-3-内存区VMA.md](./notes/section-3-内存区VMA.md) |
| 4. 缺页异常 | [notes/section-4-缺页异常.md](./notes/section-4-缺页异常.md) |
| 5. 请求调页 | [notes/section-5-请求调页.md](./notes/section-5-请求调页.md) |
| 6. COW 与堆 | [notes/section-6-写时复制与堆.md](./notes/section-6-写时复制与堆.md) |

---

## 相关

- 上一章：[chapter-08-memory-management/](../chapter-08-memory-management/)
- 下一章：[chapter-10-system-calls/](../chapter-10-system-calls/)
- 深潜：[07 Gorman Ch 4](../../06-linux-mm/) · [Ch 3 fork/COW](../chapter-03-processes/notes/section-6-创建与销毁.md)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
