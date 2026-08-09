## 读路径与写路径（衔接）

```
read(path)
    ▼
VFS（Ch 13）─► 查 address_space / 页缓存
    ├─ 命中 ──► 拷贝到用户缓冲（零拷贝/mmap 可优化）
    └─ 未命中 ──► 读盘（Ch 14 bio）─► 填入页缓存

write(path)
    ▼
页缓存（可能 COW，Ch 3/15）─► 标脏 ──► flusher 异步写回
```

| 绕过页缓存 | **`O_DIRECT`** — DB/自管缓冲 · HFT 大数据文件有时 mmap + mlock |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** read() 命中 page cache 和 miss 的性能差异有多大？

<details><summary>答案</summary>

Hit：page cache → copy_to_user → ~1-5μs。Miss：page cache 未命中 → bio → IO 调度 → 磁盘 → DMA → page cache → copy_to_user → NVMe ~50-100μs，SATA ~1-10ms。HFT 启发：1) 预读（readahead/madvise WILLNEED）让数据提前进 cache；2) mmap 零拷贝跳过 copy_to_user；3) O_DIRECT 绕过 page cache（自管理 buffer）。

</details>

</details>
---
