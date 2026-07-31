# Ch 2 内核入门 · Getting Started with the Kernel

> **Linux Kernel Development 3rd** · Robert Love · **实操入门**

> 本章定位：**拿源码 → 认目录 → 配置编译安装**；并牢记 **内核开发 ≠ 用户态 C**。  
> §2.4 钉死：主线 **≥5.18 默认 `-std=gnu11`**（此前 `gnu89`）；**`gnu11` ≠ `c11`**；**GNU C ≠ glibc**；编译器 **主流 GCC，Clang 官方支持**。  
> 另：**K&R 第2版 = C89**（≠ C99/C11）；与旧内核 `gnu89` 基底对齐。  
> 拓展⑤：编译镜像如何进 UEFI、与用户态 ELF 分界。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 获取源码** | 版本定论 + tar/Git | **主树 7.1.5** · 书 **2.6.34** · 勿 `/usr/src` |
| **② 源码树** | `arch` `drivers`… + 目录↔章对照 | 按子系统找代码 |
| **③ 编译安装** | 工具 → config → make → install | **须在 Linux/WSL**；Win 树只读 |
| **④ 开发差异** | Beast of a Different Nature | **GNU C · 无 libc · 小栈 · 同步 · 无 FP** |
| **⑤ ELF/UEFI**（拓展） | PE vs ELF · 启动链路 | 固件认 PE；内核后认 ELF |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 获取内核源码 | [notes/section-2.1-获取内核源码.md](./notes/section-2.1-获取内核源码.md) |
| 内核源码树 | [notes/section-2.2-内核源码树.md](./notes/section-2.2-内核源码树.md) |
| 编译和安装内核 | [notes/section-2.3-编译和安装内核.md](./notes/section-2.3-编译和安装内核.md) |
| 内核开发的特点 | [notes/section-2.4-内核开发的特点.md](./notes/section-2.4-内核开发的特点.md) |
| ELF 与 UEFI 启动链路（拓展） | [notes/section-2.5-ELF与UEFI启动链路.md](./notes/section-2.5-ELF与UEFI启动链路.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 怎么拿源码？ | 首选 **国内镜像 tar.xz**（见 §2.1）；Git 易断 |
| 主树版本？ | **7.1.5** 日常；书 **2.6.34** 考古 |
| 关键目录？ | **`arch` `drivers` `fs` `kernel` `mm` `include` `net`** |
| 怎么编？ | Linux/WSL：`menuconfig` → `make -j` → `modules_install` |
| UEFI 认什么？ | **PE32+ `.efi`**；内核跑起来后用户态是 **ELF**（§2.5） |
| 内核用什么 C？ | **≥5.18：`-std=gnu11`**；**≤5.17：`gnu89`**（不是纯 ISO C） |
| `gnu11` = C11？ | **否** — `gnu` = 开 GCC 扩展；基准是 C11，≠ `-std=c11` |
| GNU C = glibc？ | **否** — 方言（编译期）≠ 用户态库（运行期）；内核不链 libc |

GNU C 小实验：[`code/gnu_c_extension_demo.c`](./code/gnu_c_extension_demo.c)

---

## 本章学习目标 · 自检

- [ ] 说清本机 **7.1.5** 路径与验收项（§2.1）
- [ ] 能按目录找到 LKD 对应章（§2.2）
- [ ] 知道 Windows 树只读、真编译在 Linux
- [ ] 分清 **UEFI=PE** vs **Linux 后=ELF**
- [ ] 分清 **ISO C / GNU C / glibc**；能举 `typeof`、`({ })`、`__attribute__`
- [ ] 分清 **编译器（GCC/Clang）** vs **语言模式（`-std=gnu11`）**；学习优先 GCC

---

## 相关章节

- 上一章：[../chapter-01-intro/](../chapter-01-intro/)
- 下一章：[../chapter-03-process-management/](../chapter-03-process-management/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
