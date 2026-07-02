# Ch 17 · 文件系统

> **原书第 17 章** · HFT **🟡** · 官方源码标签 `osbook_day17`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **持久化第一步：** **FAT · BPB · Directory Entry** · **Block I/O 卷镜像** · **`ls`**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 概念** | **文件系统** · 为何选 **FAT** | 字节串 → 命名块 |
| **② 元数据** | **BPB** · **32B 目录项** | 簇 · 8.3 名 · 属性 |
| **③ 读盘** | **UEFI Block I/O** · 引导阶段 **预读卷** | 无自研 USB/SSD 驱动也能解析 |
| **④ 命令** | **`ls`** 根目录遍历 | Ch18 **读应用** 前置 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 文件系统与 FAT 选型 | [notes/section-2-文件系统与FAT选型.md](./notes/section-2-文件系统与FAT选型.md) |
| 3. BPB 参数块解析 | [notes/section-3-BPB参数块解析.md](./notes/section-3-BPB参数块解析.md) |
| 4. 目录条目结构 | [notes/section-4-目录条目结构.md](./notes/section-4-目录条目结构.md) |
| 5. UEFI Block I/O 与卷镜像 | [notes/section-5-UEFI-Block-IO与卷镜像.md](./notes/section-5-UEFI-Block-IO与卷镜像.md) |
| 6. ls 命令与小结 | [notes/section-6-ls命令与小结.md](./notes/section-6-ls命令与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **内核解析 FAT** · 引导器 **Block I/O 预读** · **`ls` 列根目录** |
| 与 02 川合 OS 对照？ | 01 **软盘 FAT 自研读盘**；Mikan **UEFI 代读 + 内存卷** |
| 与 Linux / CSAPP 对照？ | **VFS 前奏** — 08 **TLPI 文件 I/O** · ext4 对照自学 |

**本章目的：** 赋予 OS **理解磁盘布局的「慧眼」** — 迈出 **持久化外部数据** 第一步

---

## 本章学习目标 · 自检

- [ ] 说清 **BPB** 关键字段（扇区大小 · 簇大小 · **根目录起始簇**）
- [ ] 解析 **32 字节 Directory Entry**（8.3 · 属性 · 起始簇）
- [ ] 解释 **Block I/O 预读** 为何在 **bootloader** 完成
- [ ] **`ls`** 如何过滤空项 / 长文件名项

---

## 相关

- 上一章：[../chapter-16-commands/](../chapter-16-commands/)
- 下一章：[../chapter-18-apps/](../chapter-18-apps/)
- 前置：[../chapter-01-hello-world/](../chapter-01-hello-world/) · [../chapter-02-edk2-memmap/](../chapter-02-edk2-memmap/) · [../chapter-03-bootloader-display/](../chapter-03-bootloader-display/)
