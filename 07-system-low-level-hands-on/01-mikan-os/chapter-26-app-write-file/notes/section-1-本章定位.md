## 1. 本章定位

> **《从零自制操作系统》Ch 26 使用应用写入文件**

---

### 一、从 Ch25 到「完整 stdio」

| Ch 25 | **Ch 26** |
|-------|-----------|
| **OpenFile / ReadFile** | **Write · O_CREAT** |
| **fat 专用 fd** | **Terminal + FAT 统一继承** |
| **grep 读文件** | **grep/readfile 读 stdin · cp 写盘** |

**跨越：** 只读 FS → **读写 + 标准输入输出** — **Unix I/O 模型** 成形。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **FileDescriptor 继承** | **fat:: / Terminal** 派生 |
| **fd 0/1/2** | **stdin · stdout · stderr** |
| **Echo · @stdin · Ctrl+D** | **键盘作文件** |
| **O_CREAT · ExtendCluster** | **新建文件 · 扩目录** |
| **Write · AllocateClusterChain** | **写穿簇链** |
| **cp** | **读+写 集大成** |

---

### 三、后续

```
Ch26 写文件 + stdio  ← 本章
    ↓
Ch27 应用内存 · Ch29 IPC
```

→ [Ch25 读文件](../chapter-25-app-read-file/)

---

← [Ch 26 导读](../README.md) · 下一节 [2. fd 继承](./section-2-FileDescriptor继承与终端fd.md)
