# 《从零自制操作系统》· 全书目录

> **内田公太** · 日版 *ゼロからの OS 自作入門* · 构建 **MikanOS**  
> **结构：** 第 **0–31 章** 主体 + **附录 A–F**（共 6 个附录）  
> **官方目次：** [zero.osdev.jp/toc.html](http://zero.osdev.jp/toc.html)

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 启动链 / 分页 / 中断 / 调度 / syscall — 与热路径底层强相关 |
| 🟡 | GUI / FS / 驱动 — 加深工程结构，有余力再读 |
| ⚪ | 应用与界面功能 — 时间紧可后补 |

---

## 主体章节（第 0–31 章）

### 第 0–6 章 · UEFI 启动与输入

| 章 | 中文标题 | 目录 slug（规划） | 标签 |
|----|----------|-------------------|------|
| 0 | 个人可以制作操作系统吗 | `chapter-00-intro` | 🟡 |
| 1 | 计算机工作原理和 Hello World | `chapter-01-hello-world` | 🔴 |
| 2 | EDK II 和内存映射 | `chapter-02-edk2-memmap` | 🔴 |
| 3 | 屏幕显示实践和引导加载器 | `chapter-03-bootloader-display` | 🟡 |
| 4 | 像素绘图和 make 入门 | `chapter-04-pixel-make` | ⚪ |
| 5 | 文本显示和控制台类 | `chapter-05-console-text` | ⚪ |
| 6 | 鼠标输入和 PCI | `chapter-06-mouse-pci` | 🟡 |

### 第 7–14 章 · 中断 · 内存 · 多任务

| 章 | 中文标题 | 目录 slug | 标签 |
|----|----------|-----------|------|
| 7 | 中断和 FIFO | `chapter-07-interrupt-fifo` | 🔴 |
| 8 | 内存管理 | `chapter-08-memory` | 🔴 |
| 9 | 叠加过程 | `chapter-09-layers` | ⚪ |
| 10 | 窗口 | `chapter-10-window` | ⚪ |
| 11 | 定时器和 ACPI | `chapter-11-timer-acpi` | 🔴 |
| 12 | 键盘输入 | `chapter-12-keyboard` | ⚪ |
| 13 | 多任务处理（1） | `chapter-13-multitask1` | 🔴 |
| 14 | 多任务处理（2） | `chapter-14-multitask2` | 🔴 |

### 第 15–20 章 · 终端 · 文件系统 · 分页 · 系统调用

| 章 | 中文标题 | 目录 slug | 标签 |
|----|----------|-----------|------|
| 15 | 终端 | `chapter-15-terminal` | ⚪ |
| 16 | 命令 | `chapter-16-commands` | ⚪ |
| 17 | 文件系统 | `chapter-17-filesystem` | 🟡 |
| 18 | 应用 | `chapter-18-apps` | ⚪ |
| 19 | 分页 | `chapter-19-paging` | 🔴 |
| 20 | 系统调用 | `chapter-20-syscall` | 🔴 |

### 第 21–31 章 · 窗口应用 · 图形 · 文件 I/O · IPC

| 章 | 中文标题 | 目录 slug | 标签 |
|----|----------|-----------|------|
| 21 | 窗口应用 | `chapter-21-window-apps` | ⚪ |
| 22 | 图形和事件（1） | `chapter-22-graphics-events1` | ⚪ |
| 23 | 图形和事件（2） | `chapter-23-graphics-events2` | ⚪ |
| 24 | 多终端 | `chapter-24-multi-terminal` | ⚪ |
| 25 | 使用应用读取文件 | `chapter-25-app-read-file` | 🟡 |
| 26 | 使用应用写入文件 | `chapter-26-app-write-file` | 🟡 |
| 27 | 应用的内存管理 | `chapter-27-app-memory` | 🔴 |
| 28 | 日文显示和重定向 | `chapter-28-japanese-redirect` | ⚪ |
| 29 | 应用间通信 | `chapter-29-ipc` | 🟡 |
| 30 | 额外应用 | `chapter-30-extra-apps` | ⚪ |
| 31 | 前方的路 | `chapter-31-road-ahead` | 🟡 |

---

## 附录（A–F）

| 附录 | 中文标题 | 本仓库 |
|------|----------|--------|
| A | 配置开发环境 | [SETUP.md](./SETUP.md) |
| B | 获取 MikanOS | 官方 [os-from-zero](https://github.com/uchan-nos/os-from-zero) |
| C | EDK II 文件说明 | （读 Ch 2 时对照） |
| D | C++ 中的模板 | （读内核 C++ 时对照） |
| E | iPXE | 网络启动选读 |
| F | ASCII 码表 | 速查 |

---

## HFT 精读捷径

```
Ch 0–2   UEFI + 内存 map（现代启动链）
Ch 7–8   中断 + 内存管理
Ch 11    定时器 / ACPI
Ch 13–14 多任务
Ch 19–20 分页 + 系统调用  ← 与 CSAPP Ch9 / LKD / TLPI 交叉
Ch 29    应用间通信（有余力）
附录 A   环境 → SETUP.md
```

→ 前置建议：[01 30 天 OS](../01-30days-os/) Day 1–15 · 学习计划 [LEARNING_PLAN.md](./LEARNING_PLAN.md)

---

**笔记文件：** 随学习进度在 `chapter-XX-slug/notes/` 下增补；尚未创建的章仅保留上表 slug 规划。
