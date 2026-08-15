# 8. 工具选型速查

| 症状 | 优先工具 |
|------|----------|
| 不知访问了哪些文件 | `opensnoop`、`filetop` |
| 逻辑 I/O 慢 | `fileslower` |
| 缓存是否够用 | `cachestat` |
| 写延迟尖刺 | `writeback`、`ext4dist`/`xfsdist` |
| 预读浪费 | `readahead` |
| 路径查找慢 | `dcsnoop`、`dcstat` |
| mmap 文件 | `mmapfiles`、`fmapfault` |
| 临时文件风暴 | `filelife` |
| FS 内部慢操作 | `ext4slower` / `xfsslower` |

---
