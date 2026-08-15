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

| 观测 | `sar -v` dentry/inode cache — [SysPerf §8.6](../../../../15-systems-performance/chapter-08-file-systems/notes/section-8.6-观测工具.md) |

**HFT：** 日志/配置 **冷路径** 才关心 dcache；热路径 **已打开 fd** 或 **`mmap`** 绕过反复路径解析。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** dentry cache 如何加速路径解析？

<details><summary>答案</summary>

解析 `/home/user/file.txt` 需要逐级查找：`/` → `home` → `user` → `file.txt`，每级需要读目录项（磁盘 IO）。dentry cache 缓存已解析的路径组件，下次访问同一路径直接从内存中查找，O(1)。热门路径（如 /proc/cpuinfo）几乎永远在 dcache 中。dcache 还通过 hash table 加速查找。

</details>

**Q2.** 为什么 dentry 不能直接释放？reference count 如何工作？

<details><summary>答案</summary>

dentry 有引用计数：每次路径解析经过 dentry 时 +1，结束时 -1。dentry 还有 dcache LRU：即使引用计数为 0 也不立即释放（保留在 LRU 中），下次访问直接命中。内存紧张时从 LRU 尾部回收。这就是为什么 `ls /` 第二次比第一次快。

</details>

</details>
---
