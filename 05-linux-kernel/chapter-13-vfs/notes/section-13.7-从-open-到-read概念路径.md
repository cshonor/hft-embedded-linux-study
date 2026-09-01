## 从 open 到 read（概念路径）

```
open("/path/file", O_RDONLY)
    ▼
路径 walk ──► dcache 命中？ ──► inode lookup
    ▼
分配 struct file ──► 填入 files_struct->fd[]
    ▼
read(fd, ...)
    ▼
VFS sys_read ──► file->f_op->read ──► ext4_file_read / …
    ▼
（可能）页缓存 Ch 16 ──► 块层 Ch 14
```

#### v6.6 真实函数链（核对 fs/open.c、fs/namei.c、fs/read_write.c）

```
open 路径：
  SYSCALL_DEFINE3(open)
    → do_sys_openat2()            fs/open.c:1406  从用户态拷路径串
    → do_filp_open()              fs/open.c:1422
        → path_openat()           fs/namei.c:3777
             入口即 flags | LOOKUP_RCU   ← 先赌 RCU-walk
             → link_path_walk()   逐级解析（dcache 命中则不碰 FS）
             → 失败 → 丢掉重来走 ref-walk（拿引用的慢速版）
        → alloc_empty_file()      新 struct file（file_cachep slab）
        → do_dentry_open()        填 f_op（从 inode 取）、权限、调 f_op->open
    → fd_install()                fdt->fd[fd] = file（此后用户态可见）

read 路径：
  ksys_read()
    → vfs_read()                  fs/read_write.c:450
        if (f_op->read)            遗留槽（老驱动）
        else → new_sync_read()
             → call_read_iter()
             → f_op->read_iter()   ← 现代路径（kiocb + iov_iter）
                  → ext4/generic_file_read_iter
                       → 页缓存查命中（Ch16）
                            命中：copy_page_to_iter 直拷用户态
                            未命中：block IO（Ch14）→ 填缓存 → 再拷
```

| 与概念图的差异 | 说明 |
|----------------|------|
| `f_op->read` 已是遗留分支 | 现代主路径是 read_iter（见 [13.3](./section-13.3-操作对象.md)） |
| walk 一开始就是 RCU 模式 | dcache 全命中时 open **全程无锁** |
| file 的 f_op 来自 inode | open 时决定，read 时只查表——**多态解析发生在 open，read 只是执行** |

#### 这条链上每一跳的「贵」在哪（HFT 排障速查）

| 桶 | 成本量级 | 说明 |
|----|----------|------|
| 路径串拷贝 + walk | 亚微秒（dcache 全命中） | open 的固定税；RCU 失败重走翻倍仍很快 |
| 首次 lookup（dcache miss） | 微秒~毫秒 | 要走 inode_operations->lookup 甚至磁盘 |
| alloc_empty_file + fd_install | ~百纳秒 | slab + fdt 位图更新 |
| read 命中 page cache | ~1μs 级 | 拷贝为主 |
| read 未命中 | 毫秒+ | 块设备 IO——热路径上唯一「不可接受」量级 |

> 结论落地：**热路径的正确姿势是启动时 open 一次 + mmap 或长持 fd**，把上表前四行的成本全部移出热循环；剩下唯一要防的是 read 未命中（配置/行情文件 mmap 后由内核按缺页填页，慢在首次触碰）。

→ 深入下一层：[Ch 16 页缓存](../../chapter-16-page-cache/) · [Ch 14 块层](../../chapter-14-block-io/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** open("/path/file", O_RDONLY) 到 read(fd, buf, n) 的完整路径是什么？

<details><summary>答案</summary>

open: 1) 路径解析走 VFS dcache → 2) 找到 inode → 3) 权限检查 → 4) 分配 file 结构 → 5) fd 表中分配 fd → 返回 fd。read: 1) fd → file → file_operations->read → 2) 检查 page cache → 3) 命中则拷贝到用户态；未命中则发起磁盘 IO → 4) 数据进入 page cache → 5) 拷贝到用户 buf。HFT 用 mmap 跳过 read 的拷贝步骤。

</details>

**Q2.** `path_openat` 为什么一进来就带 `LOOKUP_RCU`？如果 walk 到一半有人 rename 了目录怎么办？

<details><summary>答案</summary>

先赌「无人竞争」：RCU-walk 全程不拿 dentry 锁/引用，靠 RCU 宽限期保证对象活着 + seqcount 检测一致性——最快。中途有人 rename：序列号变化被发现后**不定位失败点，直接整趟作废**，退回 ref-walk（逐级拿引用的慢速版）重走。赌赢是常态（路径解析极少撞上 rename），赌输也只是慢一次。这是「乐观并发 + 慢路径兜底」模板（同 per-VMA lock）。

</details>

**Q3.** 同一个文件 open 两次得到 fd3、fd4——两个 fd 的 read 各自的偏移互不干扰，但 `mmap` 同一文件两处共享页缓存。为什么？

<details><summary>答案</summary>

偏移（f_pos）住在 **file** 对象里——每次 open 一个新 file，天然隔离；所以 fd3/fd4 各读各的。页缓存挂在 **inode 的 address_space**（`inode->i_mapping`）上——全局一份，与谁打开无关；fd3 读入的页，fd4（甚至别的进程）read/mmap 同一区间直接命中。四对象模型直接回答这类问题：**"每个 X 一份"取决于状态挂在哪个对象上**——file 挂进程态（偏移/标志），inode 挂全局态（数据/元数据），dentry 挂路径态（名字）。判断任何 VFS 行为先问"状态住在哪"。

</details>

</details>
---
