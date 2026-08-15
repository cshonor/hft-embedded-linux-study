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



<details>
<summary>自测题（点击展开）</summary>

**Q1.** file_system_type 和 super_block 的关系？挂载文件系统时发生了什么？

<details><summary>答案</summary>

每个文件系统类型（ext4/nfs/proc）注册一个 file_system_type（含 mount 函数）。mount 时内核调用该 FS 的 mount 函数 → 读取超级块 → 创建 super_block → 构建 inode 树。super_block 是挂载实例，file_system_type 是类型描述。一个 FS 类型可以挂载多次（多个 super_block）。

</details>

</details>
---
