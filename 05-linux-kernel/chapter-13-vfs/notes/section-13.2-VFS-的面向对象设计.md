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

#### 纯 C 怎么做 OOP：嵌入 + 操作表（与 Ch6 同源）

VFS 的对象模型和 [Ch6.1 设计原则](../../chapter-06-kernel-data-structures/notes/section-6.1-设计原则.md)是同一套机制，只是规模最大：

| OOP 概念 | C 实现 | 内核证据（v6.6） |
|----------|--------|------------------|
| 类 = 数据 + 方法 | 结构体**内嵌函数指针表**：`inode->i_op`、`file->f_op`、`dentry->d_op`、`sb->s_op` | `include/linux/fs.h` 各结构含 `const struct *_operations *` |
| 继承 | **组合代替**：对象持一张表指针，换表 = 换"类" | 同一 inode 类型可换 f_op（如 O_TMPFILE） |
| 多态调用 | `file->f_op->read_iter(...)` — 一行代码路由到任意 FS | `fs/read_write.c` 全是这个模式 |
| 接口（interface） | 第四张表 `address_space_operations`（页缓存回调） | `inode->i_mapping->a_ops` |
| 构造/析构 | `alloc_inode`/`destroy_inode`（在 super_operations 里！） | **连"生灭"都交给子类**——slab 来自各 FS 自己的缓存 |

> 为什么不用 C++？内核 1991 年选型时的现实约束（编译器不稳）变成了**文化约束**：显式的函数指针调用可 grep、可 ftrace（`f_op->read_iter` 能被 kprobe 挂上）、无隐式构造/异常。代价是没有编译期类型检查拼错函数名要运行时才发现——不过操作表是静态 const 结构体，漏填的槽位是 NULL，调用处有 `if (f_op->read_iter)` 守卫。

#### 四对象 × 生命周期与缓存归属（谁管谁回收）

| 对象 | 诞生于 | 缓存池 | 回收触发 |
|------|--------|--------|----------|
| superblock | mount 时 FS 的 `mount()` 回调 | 各 FS 自建 sb 缓存 | umount / 内存压力（可回收的 FS） |
| inode | 首次路径解析 lookup | 各 FS 的 inode slab（Ch12.2） | 引用归零 + 缓存收缩 |
| dentry | 路径解析中创建 | **全局 dcache**（统一 slab） | 引用归零后进 LRU，内存压力逐出 |
| file | open 成功时（`alloc_empty_file`） | 全局 filp slab | close 最后一个引用时立即销毁（**不缓存**） |

> 注意 file 是四对象里唯一**不进缓存**的——它没有"重用"价值（偏移/标志都是进程私有的），close 即死。这决定了 fd 的高频 open/close 是 slab 分配器压力（Ch12），而 dcache 是全局共享的。

→ **Ch 16** 页缓存挂在 address_space / inode 侧



<details>
<summary>自测题（点击展开）</summary>

**Q1.** VFS 的四大核心对象是什么？它们的关系？

<details><summary>答案</summary>

super_block（文件系统实例）、inode（文件元数据）、dentry（目录项/路径缓存）、file（打开的文件实例）。关系：super_block → 管理该 FS 的所有 inode；inode → 对应一个文件；dentry → 构成路径树，指向 inode；file → 进程打开文件后创建，指向 dentry。一个 inode 可被多个 dentry 引用（硬链接），一个 dentry 可被多个 file 引用（多次 open）。

</details>

**Q2.** 为什么说 VFS 的面向对象是「组合而非继承」？换掉 `file->f_op` 意味着什么？

<details><summary>答案</summary>

C 没有继承，VFS 用**对象持有操作表指针**实现"类"：方法不编译进对象，而是一张独立的 const 函数指针表。换表指针 = 运行时换"类"——典型例子是 open 一个设备文件时，`do_dentry_open` 根据 inode 类型把 f_op 从 def_fifo_fops/def_chr_fops 换成具体驱动注册的表；epoll 内部也用换 f_op 实现 file 的"代理"（eventfd 被包装）。继承语义上做不到的（如让 ext4 file 同时是 pipe），组合天然支持不了的组合语义，多态反而更灵活——代价是少一层编译期保障。

</details>

**Q3.** 四个对象里为什么只有 file 不进任何缓存、close 即销毁？

<details><summary>答案</summary>

superblock/inode/dentry 都携带**可复用的全局状态**：sb 有 FS 元数据，inode 有文件元数据（下次 open 还要用），dentry 有路径解析结果（下次走同路径还要用）——缓存它们能省真实的 IO/查找成本。file 携带的是**纯进程私有状态**（f_pos 偏移、O_APPEND 等标志、属主 cred）：下一个 open 的人要的是全新状态，旧 file 对任何人都无复用价值，留着只占内存。所以 close 最后一个引用时直接回 slab。推论：高频 open/close 的成本在 slab 分配/释放 + fdtable 更新，不在"缓存查找"——想省这个成本就该长持 fd（HFT 配置文件读一次挂 forever）。

</details>

</details>
---
