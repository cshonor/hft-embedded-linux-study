## ② VFS 的面向对象设计 · 四大对象

内核用 **纯 C**，VFS 用 **对象 + 操作表** 模拟 OOP：

| 对象 | 代表 | 主要内容 |
|------|------|----------|
| **超级块 superblock** | 一个 **已挂载** 的文件系统实例 | FS 级控制信息（块大小、魔数、挂载状态…） |
| **索引节点 inode** | 一个 **具体文件**（含目录） | **元数据**：权限、大小、所有者、时间戳… **不含文件名** |
| **目录项 dentry** | 路径中的 **一段名字**（目录或文件名） | 路径解析组件；**目录也是文件** |
| **文件 file** | 进程 **已打开** 的文件 | **进程视角**：打开模式、**f_pos 偏移**、标志… |

#### 关系简图

```
挂载点 /dev/sda1
    └── superblock (ext4 实例)
            └── inode (文件元数据)
                    ▲
            dentry ("foo.c" 这一节名字) ──► 路径链 /home/.../foo.c
                    ▲
            file (某进程 fd=3 打开它) ──► 当前读写偏移
```

| 区分 | |
|------|--|
| **inode** | 「这个文件是什么」— 全局 FS 内一份 |
| **file** | 「这个进程怎么用它」— 每打开一次一份 |

→ **Ch 16** 页缓存挂在 address_space / inode 侧



<details>
<summary>自测题（点击展开）</summary>

**Q1.** VFS 的四大核心对象是什么？它们的关系？

<details><summary>答案</summary>

super_block（文件系统实例）、inode（文件元数据）、dentry（目录项/路径缓存）、file（打开的文件实例）。关系：super_block → 管理该 FS 的所有 inode；inode → 对应一个文件；dentry → 构成路径树，指向 inode；file → 进程打开文件后创建，指向 dentry。一个 inode 可被多个 dentry 引用（硬链接），一个 dentry 可被多个 file 引用（多次 open）。

</details>

</details>
---
