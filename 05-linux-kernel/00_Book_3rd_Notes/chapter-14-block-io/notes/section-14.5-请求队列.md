## ⑤ 请求队列 · Request Queues

| 结构 | 角色 |
|------|------|
| **`request_queue`** | 每块设备一条（书中 **单队列** 模型）— 挂起上层来的 I/O |
| **`struct request`** | 队列中的 **一个 I/O 请求** — 可含 **一个或多个 bio** |

```
文件系统 / 页回写
    ▼
构造 bio ──► 并入 request ──► request_queue
    ▼
I/O 调度器 ──► 驱动 ──► 磁盘
```

> **现代演进：** **blk-mq（多队列）** 替代单队列 + 每 CPU 队列 — 读书抓 **「bio → 队列 → 调度 → 驱动」** 即可。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 块 IO 请求队列如何合并和排序？对 HFT 有什么影响？

<details><summary>答案</summary>

请求队列用电梯算法（elevator）合并相邻扇区的请求（merge）+ 排序（sort），减少磁盘寻道。SSD 不需要排序（无寻道），用 none/mq-deadline 调度器。HFT 交易日志写 NVMe：如果 IO 延迟不确定（被 merge/sort 延迟），可以用 O_DIRECT + io_uring 绕过 page cache 和 IO 调度。

</details>

</details>
---
