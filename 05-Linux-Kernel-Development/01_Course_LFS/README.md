# LFS 从零构建系统 · 课程笔记

**01_Course_LFS** · [返回 02 总目录](../README.md)

> **来源：** B站 **LFS 系列视频** · **16 集**（p0–p15）· 完整覆盖 Linux From Scratch 从零搭建流程  
> **定位：** LKD 前置预习 — 内核编译、根文件系统、用户态/内核态边界的**整机构建**上下文

📋 **分集目录** → [OUTLINE.md](./OUTLINE.md) · **跟做清单/避坑** → [CHECKLIST.md](./CHECKLIST.md)

---

## 学习顺序

```
01 LFS（本文件夹）→ 02 内核编程视频 6 集 → 03 架构理论 → 00 LKD 第三版
```

→ 课书对照：[LEARNING-PATH.md § LFS](../LEARNING-PATH.md#1-lfs--编译系统上下文用户态内核态)

---

## 分集速查

| 阶段 | 集数 | 核心 |
|------|------|------|
| 概述 | p0 | LFS 流程、BIOS→MBR→内核启动 |
| 准备 | p1–p3 | 脚本、源码包、编译环境 |
| chroot | p4–p5 | 工具链、根文件系统 |
| 构建 | p6–p9 | 目录、核心工具、调试 |
| 完善 | p10–p14 | 配置、**内核编译**、GRUB |
| 演示 | p15 | BusyBox 极简系统（选看） |

| 集 | 笔记 |
|----|------|
| p0–p15 | 见 [OUTLINE.md](./OUTLINE.md) 完整链接表 |

---

## 学习价值

1. **Linux 底层** — `/bin`、`/etc`、`/dev` 从何而来；工具链与 glibc 依赖关系
2. **启动链路** — 白板级还原：BIOS → MBR → Bootloader → `vmlinuz` → init
3. **调优基础** — 亲手 `menuconfig` 编译内核 → 衔接 HFT 内核裁剪与裸机调优

---

## 前置与配套

| | 建议 |
|---|------|
| **前置** | [01-CSAPP](../../01-CSAPP-3rd/) · Linux 命令行 · Shell |
| **环境** | VMware / VirtualBox（勿在主系统实验） |
| **官方手册** | [LFS Book stable](https://www.linuxfromscratch.org/lfs/view/stable/) |

---

## 对应 LKD 第三版

| 本课（尤其 p10–p14） | 书本 |
|---------------------|------|
| 内核编译、`menuconfig` | Ch 2 内核入门 |
| 根文件系统、glibc、用户程序 | Ch 1、Ch 5 系统调用 |
| 启动与整机构建直觉 | Ch 1 简介 |

## HFT 关联

- 内核编译/配置直觉 → [10-HFT ch05 内核调优](../../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- 启动链理解 → 裸机部署、init 最小化
