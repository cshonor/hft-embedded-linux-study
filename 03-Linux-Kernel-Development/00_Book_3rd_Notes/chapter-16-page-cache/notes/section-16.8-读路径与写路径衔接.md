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

---
