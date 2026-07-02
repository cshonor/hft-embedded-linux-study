## 1. 本章定位

> **ULK Ch 12 The Virtual Filesystem** · Linux 如何 **统一** 异构文件系统

---

### 一、本章讲什么

Linux 支持 Ext2、MS-DOS、NFS 等 **底层实现各异** 的文件系统。VFS 是应用与具体 FS 之间的 **抽象桥梁**：

- 用户用同一套 **`open` / `read` / `write`**（POSIX）  
- 内核通过 **对象 + 函数指针** 分派到具体实现  
- **「一切皆文件」** 的 Unix 哲学在此落地  

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-VFS的作用.md) | 抽象接口、纯内存操作 vs 落盘 |
| [3](./section-3-四大核心对象.md) | superblock、inode、file、dentry |
| [4](./section-4-高速缓存.md) | Dentry Cache、Inode Cache |
| [5](./section-5-挂载与路径查找.md) | 命名空间、特殊 FS、`path_lookup` |
| [6](./section-6-文件锁.md) | 建议性/强制性、flock/fcntl |

---

### 三、在 Linux 链上的位置

```
Ch 1  Unix 文件系统概念
Ch 10 open/read/write syscall 入口
Ch 12 VFS（本章）— 通用对象与路径解析
Ch 16 文件访问（read/write 路径）
Ch 17 Ext2 等具体实现
```

HFT 热路径多在 **内存 / 网络 / 共享内存**；VFS 对 **配置、`/proc` 调参、日志** 仍有价值，精读优先级 ⚪。

---

← [Ch 12 导读](../README.md) · 下一节 [2. VFS 作用](./section-2-VFS的作用.md)
