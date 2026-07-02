# Ch 15 页高速缓存 · The Page Cache

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **⚪ 选读**  
> 磁盘数据 RAM 副本 — `address_space`、基数树、脏页回写

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. address_space | [notes/section-2-页缓存与address_space.md](./notes/section-2-页缓存与address_space.md) |
| 3. 基数树与标签 | [notes/section-3-基数树与标签.md](./notes/section-3-基数树与标签.md) |
| 4. 缓冲页 | [notes/section-4-缓冲页与buffer_head.md](./notes/section-4-缓冲页与buffer_head.md) |
| 5. 回写脏页 | [notes/section-5-回写脏页与pdflush.md](./notes/section-5-回写脏页与pdflush.md) |
| 6. 同步 syscall | [notes/section-6-同步系统调用.md](./notes/section-6-同步系统调用.md) |

---

## 相关

- 上一章：[chapter-14-block-devices/](../chapter-14-block-devices/)
- 下一章：[chapter-16-file-access/](../chapter-16-file-access/)
- 衔接：[Ch 8 struct page](../chapter-08-memory-management/) · [Ch 12 inode](../chapter-12-VFS/) · [Ch 14 bio](../chapter-14-block-devices/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
