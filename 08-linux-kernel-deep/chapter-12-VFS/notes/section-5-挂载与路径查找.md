## 5. 挂载、命名空间与路径查找

---

### 一、挂载 (Mounting)

将某 FS 的 superblock **挂到** 目录树某 **挂载点**（mount point）：

```
/                    ← 根 FS
├── tmp              ← 可能是独立分区 mount
└── proc             ← procfs mount
```

穿越挂载点时，VFS 切换到 **子 FS 的 superblock** 继续解析。

---

### 二、命名空间 (Namespaces)

| 传统 Unix | Linux 2.6+ |
|-----------|--------------|
| 单一全局目录树 | 进程可有 **独立** 挂载树视图 |

- 默认：共享 **init** 进程的命名空间  
- **`clone(CLONE_NEWNS)`**：新 **隔离** 挂载视图（容器基础能力之一）

→ 进程创建：[Ch 3 section-6](../../chapter-03-processes/notes/section-6-创建与销毁.md)

---

### 三、特殊文件系统

| FS | 特点 |
|----|------|
| **`/proc`** | 无块设备 — 暴露 **内核/进程** 数据结构 |
| **`/sys`** |  sysfs — 设备与内核属性 |
| **`pipefs`** | 管道底层支持 |

「一切皆文件」：这些也走 **同一 VFS 对象模型**。

HFT：`/proc/sys`、`/sys` 调内核参数时常用。

---

### 四、路径名查找 `path_lookup()`

进程以 **`/tmp/test`** 等路径操作文件时：

```
path_lookup()
    ↓ 逐分量
查 Dentry Cache → 权限检查 → 挂载点切换
    ↓
得到目标 dentry + inode
```

**符号链接：**

- 需 **递归** 解析链接目标  
- **最大嵌套深度 5** — 防止恶意环链导致内核 **死循环 / 栈溢出**

> **深潜可选：** `link_path_walk()`、`follow_link()` — 见 `fs/namei.c`。

---

← [4. 高速缓存](./section-4-高速缓存.md) · 下一节 [6. 文件锁](./section-6-文件锁.md)
