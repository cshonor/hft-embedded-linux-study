# 内核编程实操 · 课程笔记

**02_Course_Kernel_7Lectures** · [返回 02 总目录](../README.md)

> **来源：** B站 **内核编程系列视频** · **6 集** · 中英字幕 · 编译内核 → LKM → 驱动 → 调试  
> **定位：** LKD 前置预习 — 亲手写、加载、调试内核模块，扫清书本「LKM、中断、调试」阅读障碍

📋 **分集目录** → [OUTLINE.md](./OUTLINE.md) · **环境搭建/避坑** → [CHECKLIST.md](./CHECKLIST.md)

---

## 学习顺序

```
01 LFS → 02 本课程（6集）→ 03 架构理论 → 00 LKD 第三版
```

→ 课书对照：[LEARNING-PATH.md § 内核实操](../LEARNING-PATH.md#2-内核编程视频--6-集--模块进程中段)

---

## 分集速查

| 集 | 主题 | 笔记 |
|----|------|------|
| 1 | 编译与启动 | [episode-e01](./episode-e01-编译与启动.md) |
| 2 | Linux 内核模块 | [episode-e02](./episode-e02-Linux内核模块.md) |
| 3 | 前期准备 | [episode-e03](./episode-e03-前期准备.md) |
| 4 | Linux 驱动程序 | [episode-e04](./episode-e04-Linux驱动程序.md) |
| 5 | BusyBox 极简系统 | [episode-e05](./episode-e05-BusyBox极简系统.md) |
| 6 | 内核调试 | [episode-e06](./episode-e06-内核调试.md) |

---

## 第 1 课画面要点（编译与启动）

- `sudo pacman -S git` — Arch 系包管理
- `sudo apt install git` — Debian/Ubuntu 系（课程对比不同发行版）
- 演示环境为德语本地化 Linux；**用自己发行版对应命令即可**

---

## 学习价值

| 能力 | 对应 LKD |
|------|----------|
| 编译、启动自定义内核 | Ch 2 内核入门 |
| LKM 编写/加载/卸载 | Ch 17 设备与模块 |
| 源码树结构 | Ch 1–2 背景 |
| 驱动骨架 | Ch 17（延伸 LDD） |
| GDB/QEMU、Oops | Ch 18 调试 |

---

## 前置与配套

| | 建议 |
|---|------|
| **前置** | [01 LFS](../01_Course_LFS/) 或 Linux 命令行 + **C 语言** |
| **环境** | **QEMU** 编译/测试内核（推荐） |
| **延伸** | LKD · *Linux Device Drivers* · 内核文档 |

## HFT 关联

- e1 内核编译 → [10-HFT ch05 内核调优](../../10-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- e6 调试 → 生产 Oops/panic 分析
- e2+ → 理解模块/中断上下文 → LKD Ch 7–8、[06-Rosen Ch14](../../06-Linux-Kernel-Networking/chapter-14-高级主题.md)

## 动手代码

`code/` — 各集 LKM / Makefile 示例（待跟做时补充）
