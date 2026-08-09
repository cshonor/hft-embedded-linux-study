## 1. 本章定位

> **《从零自制操作系统》Ch 25 使用应用读取文件**

---

### 一、从 Ch24 到「应用读盘」

| Ch 17–18 | **Ch 25** |
|----------|-----------|
| 内核 **`ls`/`cat`** — 内存卷 FAT | **应用 `fopen`** — **fd + syscall** |
| 根目录 **扁平** | **目录树 · `apps/`** |
| 无 **fd 抽象** | **OpenFile / ReadFile** |

**跨越：** 应用生态 **处理持久化文本/数据** — 向 **成熟 OS** 迈进。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **FindFile 递归** | **相对/绝对路径** |
| **APPS_DIR** | 构建时 **`apps/`** |
| **fd · files_** | 每 **Task** 打开文件表 |
| **FileDescriptor** | **偏移 · 簇链 read** |
| **Newlib** | **open/read/sbrk** |
| **readfile · grep** | **stdio · `<regex>`** |

---

### 三、后续

```
Ch25 应用读文件  ← 本章
    ↓
Ch26 应用写文件
Ch29 IPC
```

→ [Ch17 文件系统](../chapter-17-filesystem/) · [Ch24 多任务 Task](../chapter-24-multi-terminal/)

---

← [Ch 25 导读](../README.md) · 下一节 [2. 目录树/ls](./section-2-目录树与ls升级.md)
