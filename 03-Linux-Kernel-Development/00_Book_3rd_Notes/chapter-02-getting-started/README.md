# Ch 2 内核入门 · Getting Started with the Kernel

> **Linux Kernel Development 3rd** · Robert Love · **实操入门**

> 本章定位：**拿源码 → 认目录 → 配置编译安装**；并牢记 **内核开发 ≠ 用户态 C** 的硬性约束。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 获取源码** | Git / tarball / patch | 别解压到 `/usr/src/linux` |
| **② 源码树** | `arch` `drivers` `fs`… | 按子系统找代码 |
| **③ 编译安装** | config → make → install | `menuconfig` · `make -j` · modules |
| **④ 开发差异** | Beast of a Different Nature | **无 libc · 小栈 · 同步 · 无 FP** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 获取内核源码 | [notes/section-2.1-获取内核源码.md](./notes/section-2.1-获取内核源码.md) |
| 内核源码树 | [notes/section-2.2-内核源码树.md](./notes/section-2.2-内核源码树.md) |
| 编译和安装内核 | [notes/section-2.3-编译和安装内核.md](./notes/section-2.3-编译和安装内核.md) |
| 内核开发的特点 | [notes/section-2.4-内核开发的特点.md](./notes/section-2.4-内核开发的特点.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 怎么拿源码？ | **`git clone` + `pull`** 优先；tarball 放用户目录 |
| 怎么打补丁？ | **`patch -p1 <`** 或 git 系列 |
| 关键目录？ | **`arch` `drivers` `fs` `kernel` `mm` `include`** |
| 怎么编？ | **`menuconfig` → `make -jN` → 装镜像 + `modules_install`** |
| 和用户态最大不同？ | **无 libc · printk · 小栈 · 无换页 · 严同步 · 慎 FP** |

---

## 本章学习目标 · 自检

- [ ] 能在 **个人目录** 完成 clone、`menuconfig`、`make -j`
- [ ] 说清 **`printk` vs `printf`**、**内核栈大小** 约束
- [ ] 知道 **`likely/unlikely`** 是分支预测而非逻辑改变
- [ ] 能指出 **`kernel/`、`mm/`、`fs/`** 各对应本书哪几章
- [ ] 理解为何 HFT 调内核参数时要 **可回滚引导项**

---

## 相关章节

- 上一章：[../chapter-01-intro/](../chapter-01-intro/)
- 下一章：[../chapter-03-process-management/](../chapter-03-process-management/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
