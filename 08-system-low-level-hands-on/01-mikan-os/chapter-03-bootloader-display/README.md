# Ch 3 · 屏幕显示实践和引导加载器

> **原书第 3 章** · HFT **🟡** · 官方源码标签 `osbook_day03`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **分水岭：** Loader / **Kernel 分离** + **GOP 像素绘图**

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | **MikanLoader 加载 `kernel.elf`**，经 **GOP** 把 **帧缓冲硬件参数**（非截图）交给 **`KernelMain()`**；Loader / 内核分离 + QEMU 调试。 |
| **全书作用** | **分水岭** — UEFI Loader 退场、**自制内核登场**；**ExitBootServices → 跳内核** 完整链在此章建立。 |
| **← 前置** | [Ch2 memmap](../chapter-02-edk2-memmap/) — 知道哪些物理段能加载内核 |
| **→ 后续** | [Ch4 像素 / make](../chapter-04-pixel-make/) — 在内核里画像素、熟悉构建系统 |

---
### 本章四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 调试** | QEMU 监视器 · 寄存器 | **RIP / RFLAGS** — 改代码 Bug 会变时的排错手段 |
| **② 内核** | `kernel.elf` · **ELF** · `hlt` 循环 | 第一个 **独立内核**（非 UEFI 应用） |
| **③ 加载** | 读文件 · 分配页 · 解析入口 · 跳转 | **MikanLoader 核心使命** |
| **④ 显示** | **GOP** · Frame Buffer → **`KernelMain()`** | 内核接管 **像素级** 屏幕 |

---

**核心通读：** [完整流程 · MikanLoader + GOP → kernel main](./notes/section-1-完整流程Miniload-GOP-kernel.md)（建议 **先读**）

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| **0. 完整流程** | [**Miniload + GOP → kernel main**](./notes/section-1-完整流程Miniload-GOP-kernel.md)（**口述通读 · 五步法**） |
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. QEMU 监视器与寄存器 | [notes/section-2-QEMU监视器与寄存器.md](./notes/section-2-QEMU监视器与寄存器.md) |
| 3. 第一个内核与 ELF 加载 | [notes/section-3-第一个内核与ELF加载.md](./notes/section-3-第一个内核与ELF加载.md)（**索引**） |
| | [3.1 **ELF 误区与通用格式**](./notes/section-3-1-kernel.elf基础定义与核心作用.md) · [3.2 双视图](./notes/section-3-2-ELF三大结构与链接执行双视图.md) · [3.3 链接脚本](./notes/section-3-3-编译链接脚本与生成流程.md) · [3.4 readelf/GDB](./notes/section-3-4-readelf调试与常用命令.md) · [3.5 vmlinux](./notes/section-3-5-与vmlinux对比及常见问题.md) · [3.6 **六步加载**](./notes/section-3-6-MikanLoader加载流程.md) |
| 4. GOP 与帧缓冲区 | [notes/section-4-GOP与帧缓冲区.md](./notes/section-4-GOP与帧缓冲区.md) |
| 5. KernelMain 与错误处理 | [notes/section-5-KernelMain与错误处理.md](./notes/section-5-KernelMain与错误处理.md) |
| 6. 汇编指针与小结 | [notes/section-6-汇编指针与小结.md](./notes/section-6-汇编指针与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **MikanLoader 加载 `kernel.elf`**，经 **GOP** 把帧缓冲交给 **`KernelMain()`** |
| 与 02 川合 OS 对照？ | 01 **Day 4–5** 才把引导与内核分离；Mikan **Ch 3 即 ELF 加载 + 图形** |
| 与 Linux / CSAPP 对照？ | 类似 **bootloader → `_start`/`startup_64`**；帧缓冲 = 早期 **fbcon** 前驱 |

**本章目的：** Loader **加载内核 + 传递硬件信息**；OS 真正拥有 **控制像素** 的能力。

---

## 本章学习目标 · 自检

- [ ] 能复述 **§0 五步法**：Loader → ELF → GOP → ExitBootServices → KernelMain
- [ ] 说清 **MikanLoader ≈ GRUB**，**GOP 传地址不传截图**
- [ ] 说清 **ELF 是通用格式**（`ls` / `.so` / `vmlinux` 都是 ELF），**不是**「专给内核用的标签」
- [ ] 说清 **磁盘上的 `kernel.elf` ≠ OS 在跑** — 须 **搬段 + 跳 `e_entry`**
- [ ] 说清 **`.efi` 固件直接跑** vs **ELF 须 Loader 手写解析**
- [ ] 能对照 **链接视图 / 执行视图** 说明 Bootloader 读哪张表
- [ ] 会用 **`readelf -h/-l/-s`** 查入口与 PT_LOAD 段
- [ ] 描述 **读 ELF → 分配内存 → 跳入口** 流程
- [ ] 解释 **GOP**、Frame Buffer 及传给 **`KernelMain`** 的参数
- [ ] 检查 **`EFI_STATUS`** 做失败停机；能读 **`lea`/`mov`/`[]`** 与指针对应关系

---

## 相关

- 上一章：[../chapter-02-edk2-memmap/](../chapter-02-edk2-memmap/)
- 下一章：[../chapter-04-pixel-make/](../chapter-04-pixel-make/)
- 对照：[Ch1 PE vs ELF](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md) · [01 Day 4 引导](../../02-30days-os/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
