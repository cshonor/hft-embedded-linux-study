# Ch 3 · 屏幕显示实践和引导加载器

> **原书第 3 章** · HFT **🟡** · 官方源码标签 `osbook_day03`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **分水岭：** Loader / **Kernel 分离** + **GOP 像素绘图**

---

### 本章四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 调试** | QEMU 监视器 · 寄存器 | **RIP / RFLAGS** — 改代码 Bug 会变时的排错手段 |
| **② 内核** | `kernel.elf` · **ELF** · `hlt` 循环 | 第一个 **独立内核**（非 UEFI 应用） |
| **③ 加载** | 读文件 · 分配页 · 解析入口 · 跳转 | **MikanLoader 核心使命** |
| **④ 显示** | **GOP** · Frame Buffer → **`KernelMain()`** | 内核接管 **像素级** 屏幕 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. QEMU 监视器与寄存器 | [notes/section-2-QEMU监视器与寄存器.md](./notes/section-2-QEMU监视器与寄存器.md) |
| 3. 第一个内核与 ELF 加载 | [notes/section-3-第一个内核与ELF加载.md](./notes/section-3-第一个内核与ELF加载.md) |
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

- [ ] 会用 **QEMU monitor** 查看寄存器 / 内存
- [ ] 说清 **`kernel.elf`** 与 UEFI `.efi` 的分工
- [ ] 描述 **读 ELF → 分配内存 → 跳入口** 流程
- [ ] 解释 **GOP**、Frame Buffer 及传给 **`KernelMain`** 的参数
- [ ] 检查 **`EFI_STATUS`** 做失败停机；能读 **`lea`/`mov`/`[]`** 与指针对应关系

---

## 相关

- 上一章：[../chapter-02-edk2-memmap/](../chapter-02-edk2-memmap/)
- 下一章：[../chapter-04-pixel-make/](../chapter-04-pixel-make/)
- 对照：[Ch1 PE vs ELF](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md) · [01 Day 4 引导](../../02-30days-os/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
