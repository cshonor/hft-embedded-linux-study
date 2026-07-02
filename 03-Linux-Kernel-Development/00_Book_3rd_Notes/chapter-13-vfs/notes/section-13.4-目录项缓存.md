## ④ 目录项缓存 · Dentry Cache · dcache

**路径解析**（`/home/dracula/src/the_sun_sucks.c`）— 字符串遍历 + 查找，**昂贵**。

**dcache** 缓存已解析的 **dentry** → 同路径再次访问 **更快**。

#### dentry 三种状态

| 状态 | 含义 |
|------|------|
| **使用中** | VFS **正在用** |
| **未使用** | 暂不用，但 **留在缓存** 备查 |
| **负缓存（negative）** | 路径 **无效/不存在** — 缓存「没有这文件」→ **快速拒绝** 后续无效 open |

```
第一次 open 不存在文件 ──► 负 dentry 入缓存
第二次同路径 open     ──► 不必再深入 FS 查找
```

| 观测 | `sar -v` dentry/inode cache — [SysPerf §8.6](../../../../13-Systems-Performance-2nd/chapter-08-file-systems/notes/section-8.6-观测工具.md) |

**HFT：** 日志/配置 **冷路径** 才关心 dcache；热路径 **已打开 fd** 或 **`mmap`** 绕过反复路径解析。

---
