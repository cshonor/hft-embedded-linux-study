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

#### ⚠️ 版本断崖：vfsmount 已被「吸收」

LKD3rd 把 `vfsmount` 当作挂载实例的主角——**这个讲法已过时**。核对 v6.6 源码：

| 结构 | v6.6 真实形态 | 出处 |
|------|---------------|------|
| `struct vfsmount` | **瘦身成 4 个字段**：`mnt_root`（挂载树根 dentry）、`mnt_sb`（superblock 指针）、`mnt_flags`、`mnt_idmap`（id 映射，容器用户态 UID 翻译用） | `include/linux/mount.h:70` |
| `struct mount` | **真正的挂载实例**：内嵌上面的 vfsmount，再加上 `mnt_parent`、`mnt_mountpoint`、哈希链、per-CPU 引用计数（`mnt_pcp`） | `fs/mount.h:39` |

```
struct mount {                 /* 挂载树的真实节点 */
    struct hlist_node mnt_hash;        /* 挂载哈希（按 mountpoint 索引） */
    struct mount      *mnt_parent;     /* 挂在哪个挂载上 */
    struct dentry     *mnt_mountpoint; /* 挂在哪个目录上 */
    struct vfsmount   mnt;             /* ← 被"吸收"的旧主角 */
    ...
};
```

> 为什么拆两层？挂载点查找（"这个 dentry 上有挂载吗"）要频繁做哈希查询 + 引用计数，这些**热字段**放进 mount；而 VFS 各处只关心 root/sb/flags 的地方传的是**内嵌的 vfsmount 指针**，避免暴露内部布局。这是「结构自带访问模式」的又一次现身（对照 Ch6.1）。另一个 LKD 之后的演化：**idmapped mount**（mnt_idmap，v5.12+）——容器里以不同 UID 身份操作文件的基础。

#### 挂载一次的真实流程（现代内核视角）

```
mount("none", "/data", "ext4", ...)
  ▼
file_systems 链表按 name 找到 file_system_type
  ▼
调用其 ->mount() 回调（ext4 自己读磁盘超级块、建 sb + root inode）
  ▼
分配 struct mount，填 mnt_parent/mnt_mountpoint，
  mnt.mnt_sb = 新 sb，mnt.mnt_root = root dentry
  ▼
挂进全局挂载哈希 + 挂载命名空间（nsproxy->mnt_ns）
  ▼
（可见性）路径解析走到 /data 时查哈希发现挂载 → 切到新树的 root
```

| 要点 | 说明 |
|------|------|
| 挂载是**命名空间属性** | 同一 mount 树在不同 mnt namespace 可见性不同（容器基础） |
| 同一 FS 类型可挂 N 次 | 每次一个 superblock？**不一定**——同一块设备重复挂载会**复用 sb**（只读或同参数时） |
| unmount ≠ 释放 sb | sb 引用计数归零才回收；lazy umount（MNT_DETACH）更是先摘可见性 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** file_system_type 和 super_block 的关系？挂载文件系统时发生了什么？

<details><summary>答案</summary>

每个文件系统类型（ext4/nfs/proc）注册一个 file_system_type（含 mount 函数）。mount 时内核调用该 FS 的 mount 函数 → 读取超级块 → 创建 super_block → 构建 inode 树。super_block 是挂载实例，file_system_type 是类型描述。一个 FS 类型可以挂载多次（多个 super_block）。

</details>

**Q2.** 读了 LKD3rd 之后看 v6.6 源码，找不到"vfsmount 的 mnt_parent"——为什么？

<details><summary>答案</summary>

因为 vfsmount 被**降级内嵌**了：v6.6 中 `struct vfsmount`（include/linux/mount.h）只剩 mnt_root/mnt_sb/mnt_flags/mnt_idmap 四个字段，挂载树节点信息（mnt_parent、mnt_mountpoint、哈希、引用计数）全部上移到包裹它的 `struct mount`（fs/mount.h，内核私有头）。内核代码里传 `struct vfsmount *` 的地方，实际往往是某个 mount->mnt 的内嵌地址——这是「把热字段和冷字段分家、对外只暴露窄接口」的布局优化。教训与 maple tree 同款：**LKD 的结构图要按版本对号入座，2.6 的形状不能直接当 6.x 用**。

</details>

**Q3.** 同一个 ext4 分区先后挂到 /a 和 /b（都只读），会有几个 superblock？一个进程在 /a 下 write 会怎样？

<details><summary>答案</summary>

通常**一个 superblock 被两次挂载复用**：mount 时内核按 bdev+参数查找已有 sb，匹配则增加引用，不再重建。两份 struct mount（各自 mnt_parent/mountpoint 不同）共享同一个 sb 与同一批 inode——所以在 /a 和 /b 看到的是**同一份文件内容**（同一个 inode 缓存）。只读挂载下 write 会在权限/挂载标志层被拒（EROFS）。推论：切换挂载点 ≠ 换数据视图，**inode 身份是全局的**；真正要独立副本得用不同设备/快照或 idmapped mount 改身份映射。

</details>

</details>
---
