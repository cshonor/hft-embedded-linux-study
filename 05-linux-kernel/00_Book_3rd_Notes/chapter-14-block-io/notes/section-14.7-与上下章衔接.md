## 与上下章衔接

```
read() / write()
    ▼
VFS（Ch 13）
    ▼
页缓存（Ch 16）— 命中则不经块层
    ▼
未命中 / 回写 ──► bio ──► request_queue ──► I/O 调度器 ──► 驱动
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** read() 到磁盘 IO 的完整路径经过哪些层？HFT 如何绕过？

<details><summary>答案</summary>

read() → VFS → page cache（miss）→ bio → IO 调度器 → 块设备驱动 → 硬件。HFT 绕过方法：1) O_DIRECT 跳过 page cache（直接 DMA 到用户态 buffer）；2) io_uring 异步提交（不阻塞等待）；3) mmap + madvise(MADV_WILLNEED) 预读。O_DIRECT 对 NVMe 特别有效——减少一次内核拷贝。

</details>

</details>
---
