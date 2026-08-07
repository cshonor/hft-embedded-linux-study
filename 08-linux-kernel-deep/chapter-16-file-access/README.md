# Ch 16 访问文件 · Accessing Files

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **⚪ 选读**  
> VFS + 页缓存 + 块层 **大串联** — read/write、mmap、O_DIRECT、AIO

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **AIO** (`aio_read`/`aio_write`) | **io_uring** 取代 AIO（5.1+） | [io_uring](https://lwn.net/Articles/776703/) (Jens Axboe) |
| `aio_read()`/`aio_write()` | 仍存在但已不推荐新代码使用 | [io_uring and networking](https://lwn.net/Articles/810414/) |
| `epoll` | 仍存在，io_uring 可替代部分场景 | [Efficient IO with io_uring](https://kernel.dk/io_uring.pdf) |

> **原则**：AIO→io_uring 是异步 I/O 的完全重写。ULK3 的 AIO 章节已过时，io_uring 是现代高性能 I/O 的核心。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 文件访问模式 | [notes/section-2-文件访问模式.md](./notes/section-2-文件访问模式.md) |
| 3. 读写与预读 | [notes/section-3-读写与预读.md](./notes/section-3-读写与预读.md) |
| 4. 内存映射 | [notes/section-4-内存映射.md](./notes/section-4-内存映射.md) |
| 5. 异步IO | [notes/section-5-异步IO.md](./notes/section-5-异步IO.md) |
| 6. 全路径串联 | [notes/section-6-全路径串联与索引.md](./notes/section-6-全路径串联与索引.md) |

---

## 相关

- 上一章：[chapter-15-page-cache/](../chapter-15-page-cache/)
- 下一章：[chapter-17-page-reclaim/](../chapter-17-page-reclaim/)
- 衔接：[Ch 12 VFS](../chapter-12-VFS/) · [Ch 14 块层](../chapter-14-block-devices/) · [Ch 15 页缓存](../chapter-15-page-cache/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
