## ③ 缓冲区与缓冲区头 · buffer_head（历史）

块读入内存 → 放在 **缓冲区（buffer）** 中。

| 结构 | 作用 |
|------|------|
| **`struct buffer_head`** | 描述 **内存缓冲区 ↔ 磁盘物理块** 映射 |

| 2.6 前问题 | |
|------------|--|
| **笨重** | 基本 I/O 容器 |
| **拆分大 I/O** | 强迫内核把大请求拆成 **多个细碎 buffer_head** |

→ 被 **bio** 取代（④）



<details>
<summary>自测题（点击展开）</summary>

**Q1.** buffer_head 为什么被 bio 取代？

<details><summary>答案</summary>

buffer_head 是 2.4 时代的块 IO 表示，每个 buffer_head 对应一个块（512B/4KB），大 IO 需要链式多个 bh → 开销大、不灵活。bio（2.6+）用 bio_vec 数组表示任意大小的 IO，一个 bio 可以包含多个不连续的物理段（scatter-gather），更适合现代 DMA。bh 仍保留用于元数据 IO。

</details>

</details>
---
