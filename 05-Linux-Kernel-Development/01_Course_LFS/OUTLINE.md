# LFS 课程 · 分集目录（B站 · LFS 视频教程 · 16 集）

> **Linux From Scratch** · 前置预习 · [返回课程 README](./README.md)

| 标签 | 说明 |
|------|------|
| 🔴 | 核心集 · 必看必做笔记 |
| 🟡 | 重要 · 建议跟做 |
| ⚪ | 选看 · 快速演示/复习 |

## 分集索引

| 集 | 主题 | 笔记 | 阶段 | 标签 |
|----|------|------|------|------|
| p0 | 概述 · LFS 流程与启动原理 | [episode-p00](./episode-p00-概述.md) | 总览 | 🔴 |
| p1 | 准备工作 · 脚本 | [episode-p01](./episode-p01-脚本准备.md) | 准备 | 🟡 |
| p2 | 准备工作 · 下载源码包 | [episode-p02](./episode-p02-下载源码包.md) | 准备 | 🟡 |
| p3 | 准备工作 · 编译环境 | [episode-p03](./episode-p03-编译环境.md) | 准备 | 🟡 |
| p4 | 编译基础工具链 | [episode-p04](./episode-p04-基础工具链.md) | chroot 前 | 🔴 |
| p5 | 进入 chroot · 根文件系统 | [episode-p05](./episode-p05-chroot与根文件系统.md) | chroot | 🔴 |
| p6 | 系统构建 · 基础目录 | [episode-p06](./episode-p06-基础目录结构.md) | 构建 | 🔴 |
| p7 | 系统构建 · 核心工具（上） | [episode-p07](./episode-p07-核心工具安装上.md) | 构建 | 🟡 |
| p8 | 系统构建 · 核心工具（下） | [episode-p08](./episode-p08-核心工具安装下.md) | 构建 | 🟡 |
| p9 | 系统构建 · 调试 | [episode-p09](./episode-p09-系统调试.md) | 构建 | 🟡 |
| p10 | 系统完善 · 配置 | [episode-p10](./episode-p10-系统配置.md) | 完善 | 🔴 |
| p11 | 系统完善 · 内核编译（上） | [episode-p11](./episode-p11-内核编译上.md) | 完善 | 🔴 |
| p12 | 系统完善 · 内核编译（下） | [episode-p12](./episode-p12-内核编译下.md) | 完善 | 🔴 |
| p13 | 系统完善 · 引导程序 | [episode-p13](./episode-p13-引导程序安装.md) | 完善 | 🔴 |
| p14 | 系统完善 · 收尾 | [episode-p14](./episode-p14-系统收尾.md) | 完善 | 🟡 |
| p15 | BusyBox 极简 Linux 快速演示 | [episode-p15](./episode-p15-BusyBox快速演示.md) | 演示 | ⚪ |

## 阶段总览

```
p0        启动原理：BIOS → MBR → Bootloader → vmlinuz → init
p1–p3     脚本 / 源码包 / 编译环境
p4–p5     工具链 + chroot 根文件系统
p6–p9     目录结构 / 核心工具 / 调试
p10–p14   配置 / 内核编译 / GRUB / 收尾
p15       BusyBox 极简路径（选看）
```

## 学习价值（为何放在 LKD 前置）

1. **Linux 底层** — 亲手搭工具链、根文件系统、内核、引导；理解 `/bin`、`/etc`、`/dev` 由来
2. **启动链路** — BIOS → MBR → Bootloader → 内核 → init，打通「黑盒」
3. **调优基础** — 亲手编译内核与工具，理解依赖与 `menuconfig` 选项 → 衔接 HFT 内核裁剪

## 前置与配套

| 项目 | 建议 |
|------|------|
| **前置** | [01-CSAPP](../../01-CSAPP-3rd/) 基础 · Linux 命令行 · Shell 脚本 |
| **环境** | VMware / VirtualBox 虚拟机（勿在主系统直接实验） |
| **官方手册** | [LFS Book (stable)](https://www.linuxfromscratch.org/lfs/view/stable/) — 与视频互补 |
| **操作清单** | [CHECKLIST.md](./CHECKLIST.md) — 按集跟做 + 避坑 |

→ 完成后进入 [LKD Ch2](../00_Book_3rd_Notes/chapter-02-getting-started/)
