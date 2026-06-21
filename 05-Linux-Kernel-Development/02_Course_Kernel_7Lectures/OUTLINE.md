# 内核编程实操 · 分集目录（B站 · 内核编程视频教程 · 6 集）

> **Linux Kernel Programming** · 中英字幕 · [返回课程 README](./README.md)

| 标签 | 说明 |
|------|------|
| 🔴 | 核心 · 必看必做 |
| 🟡 | 重要 · 建议跟做 |
| ⚪ | 选看 · 与 LFS 有重叠 |

## 分集索引

| 集 | 主题 | 笔记 | 标签 |
|----|------|------|------|
| 1 | 编译与启动 | [episode-e01](./episode-e01-编译与启动.md) | 🔴 |
| 2 | Linux 内核模块 | [episode-e02](./episode-e02-Linux内核模块.md) | 🔴 |
| 3 | 前期准备 | [episode-e03](./episode-e03-前期准备.md) | 🟡 |
| 4 | Linux 驱动程序 | [episode-e04](./episode-e04-Linux驱动程序.md) | 🟡 |
| 5 | BusyBox 构建极简系统 | [episode-e05](./episode-e05-BusyBox极简系统.md) | ⚪ |
| 6 | Linux 内核调试 | [episode-e06](./episode-e06-内核调试.md) | 🔴 |

## 阶段路径

```
e1  编译与启动（环境、依赖、自定义内核）
e2  第一个 LKM（编写/加载/卸载）
e3  源码树结构、开发环境深化
e4  设备驱动入门
e5  BusyBox 最小系统（可与 LFS p15 对照）
e6  GDB / QEMU 内核调试
```

## 学习价值

- **完整入门路径**：编译内核 → 模块 → 驱动 → 调试
- **发行版差异**：pacman（Arch）vs apt（Debian/Ubuntu）等包管理对比
- **安全实验**：QEMU 中编译/启动内核，不破坏宿主系统

## 前置与配套

| 项目 | 建议 |
|------|------|
| **前置** | [01 LFS](../01_Course_LFS/) 或至少熟悉 Linux 命令行 + **C 语言** |
| **环境** | QEMU 虚拟机（推荐） |
| **操作清单** | [CHECKLIST.md](./CHECKLIST.md) |
| **延伸书目** | LKD 第三版 [00_Book](../00_Book_3rd_Notes/) · LDD · LKMPG |

→ 完成后 [03 架构理论](../03_Course_Kernel_Architecture/) · 书本 [LKD Ch17 模块](../00_Book_3rd_Notes/chapter-17-设备与模块.md)
