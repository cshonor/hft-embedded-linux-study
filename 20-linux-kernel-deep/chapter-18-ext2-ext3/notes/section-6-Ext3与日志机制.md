## 6. Ext3 与日志机制 (Journaling)

> Ext3 = Ext2 **磁盘布局兼容** + **日志 (JBD)**

---

### 一、为何需要日志

Ext2 崩溃后需 **`e2fsck` 全盘扫描** — 大分区 **极慢**。

Ext3 引入 **日志** — 崩溃后 **重放完整事务** → **快速一致**。

---

### 二、JBD (Journaling Block Device)

Ext3 日志由通用 **JBD** 层实现：

| 概念 | 说明 |
|------|------|
| **Log records** | FS 修改先 **写入日志区** |
| **Handle** | **原子修改** 的单元 |
| **Transaction** | 多个 handle **聚合** 为一次提交 |

**Complete 事务** — 整事务 **完整写入日志** 后才算提交成功。

崩溃恢复：`e2fsck` **重放** 日志中 complete 事务 — 无需全盘扫描。

---

### 三、三种日志模式

| 模式 | 日志内容 | 数据顺序 | 安全 vs 速度 |
|------|----------|----------|--------------|
| **journal** | 数据 + 元数据 **都** 进日志 | 最强一致 | **最安全、最慢** |
| **ordered**（**默认**） | 主要元数据进日志 | **数据先于元数据** 落盘 | 平衡 |
| **writeback** | 元数据进日志 | **不** 保证数据/元数据顺序 | **最快**；崩溃可能见 **旧数据** |

HFT：日志盘 **fsync 延迟** 与 **ordered** 语义相关 — 数据库常专用 FS + 调优。

> **深潜可选：** `journal_start_commit`、`kjournald` 提交线程 — `fs/jbd/`。

---

### 四、本章小结

```
磁盘：块组 + inode + i_block 寻址
    ↓ 挂载
内存：ext2_sb_info / ext2_inode_info + VFS
    ↓ read/write
页缓存 + 块层 bio
    ↓ Ext3
JBD 事务日志 → 崩溃快速恢复
```

---

### 五、后续章节索引

| Ch 18 主题 | 继续读 |
|------------|--------|
| VFS / 路径查找 | [Ch 12 VFS](../chapter-12-VFS/) ⚪ |
| 页缓存写回 | [Ch 15](../chapter-15-page-cache/) ⚪ |
| 进程通信 | [Ch 19 IPC](../chapter-19-ipc/) 🟡 |
| 程序加载 | [Ch 20 程序执行](../chapter-20-program-execution.md) 🟡 |
| modern FS | ext4/xfs 文档；ULK 概念仍适用 |

---

← [5. 内存结构](./section-5-Ext2内存数据结构.md) · 下一章 [Ch 19 进程通信](../chapter-19-ipc/)
