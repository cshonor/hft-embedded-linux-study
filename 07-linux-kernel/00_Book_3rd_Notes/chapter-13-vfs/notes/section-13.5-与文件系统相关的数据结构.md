## ⑤ 与文件系统相关的数据结构

| 结构 | 作用 |
|------|------|
| **`file_system_type`** | 描述一种 FS **类型**（如 ext4）— 能力、注册、`mount` 入口 |
| **`vfsmount`** | 一次 **具体挂载实例** — 挂载点、设备名、**挂载标志** |

```
file_system_type "ext4"  ──注册──► 内核 FS 列表
        │
        mount /data
        ▼
   vfsmount（/data 上的 ext4 实例）──► superblock
```

---
