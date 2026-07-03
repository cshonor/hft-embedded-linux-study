# Ch 2 · EDK II 和内存映射

> **原书第 2 章** · HFT **🔴** · 官方源码标签 `osbook_day02`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **规范化开发起点：** EDK II 框架 + **物理内存摸底**（Memory Map）

---

## 本章定位 · 前后章关系

| | |
|---|---|
| **本章干什么** | 用 **EDK II** 重写 Hello → **MikanLoader**；调用 **`GetMemoryMap()`** 导出 **memmap CSV**；理解 **UEFI 内存类型** 与 C 指针。 |
| **全书作用** | **启动链第二环** — 从「能打印」到「能读物理世界账本」；Ch8 物理分配、Ch19 分页都依赖本章的 **Conventional / MMIO** 直觉。 |
| **← 前置** | [Ch1 Hello World](../chapter-01-hello-world/) — 已理解 UEFI 七步与 `BOOTX64.EFI` |
| **→ 后续** | [Ch3 Loader 加载内核](../chapter-03-bootloader-display/) — 用 Loader 读 ELF、跳 **kernel.elf** |

---
### 本章三段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① EDK II** | `<Uefi.h>` 重写 Hello World → **MikanLoader** | 专业 UEFI 开发工具链与库 |
| **② 内存映射** | `gBS->GetMemoryMap()` · 导出 **memmap** CSV | 物理内存「一维地图」 |
| **③ 指针基础** | `*` / `->` / 指针的指针 | 读懂 UEFI 协议接口代码 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. EDK II 与 MikanLoader | [notes/section-2-EDK-II与MikanLoader.md](./notes/section-2-EDK-II与MikanLoader.md)（**索引**） · [Ch1 §7](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) |
| | [2.1](./notes/section-2-1-EDK-II是什么与行业定位.md) · [2.2 Hello](./notes/section-2-2-用EDK-II重写HelloWorld.md) · [2.3 MikanLoader](./notes/section-2-3-MikanLoader是什么.md) · [2.4 Boot/RT](./notes/section-2-4-Boot与Runtime服务.md) · [2.5 CLANGPDB](./notes/section-2-5-CLANGPDB与自检.md) |
| 3. 主存储器与内存映射 | [notes/section-3-主存储器与内存映射.md](./notes/section-3-主存储器与内存映射.md)（**索引**） |
| | [3.1](./notes/section-3-1-内存映射指什么与RAM视图.md) · [3.2 四层](./notes/section-3-2-RAM四层占用.md) · [3.3 固件 vs EFI](./notes/section-3-3-固件与EFI应用内存隔离.md) · [3.4 **五类内存逐行解读**](./notes/section-3-4-地址清单与UEFI内存类型.md) · [3.5 自检](./notes/section-3-5-与mmap区别与自检.md) |
| 4. GetMemoryMap 与导出 memmap | [notes/section-4-GetMemoryMap与导出memmap.md](./notes/section-4-GetMemoryMap与导出memmap.md) |
| 5. C/C++ 指针基础 | [notes/section-5-C指针基础.md](./notes/section-5-C指针基础.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **MikanLoader** — 用 EDK II 简化 Hello World，并 **导出 UEFI 内存映射 CSV** |
| 与 02 川合 OS 对照？ | 01 后期才深入内存；Mikan **Ch 2 即摸底物理 RAM** — 为分页/分配打基础 |
| 与 Linux / CSAPP 对照？ | 类似 `/proc/iomem` / 固件 e820 — 对照 [CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) 虚拟地址 **之前** 的物理布局 |

**本章目的：** 步入 **EDK II 规范化开发**，在 OS 接管硬件前 **摸清内存状态**。

---

## 本章学习目标 · 自检

- [ ] 能区分 **UEFI 规范 / EDK II 工具链 / 手写 uefi.h** —— UEFI **不是** C 第三方库
- [ ] 能说出 **MikanLoader = Boot Loader**，**不是** 内核
- [ ] 能说出 Loader **加载两类东西**：**kernel 镜像** + **内存/硬件描述（GetMemoryMap、GOP…）**
- [ ] 能描述 **Loader 生命周期**：EfiMain → BootServices → ExitBootServices → 跳内核
- [ ] 解释 **内存映射** — 哪些物理地址段空闲、哪些已被 UEFI 占用
- [ ] 描述 **`gBS->GetMemoryMap()`** 与 **memmap CSV** 导出流程
- [ ] 会用 **`->`** 访问结构体成员，理解 UEFI 中 **指针的指针** 常见模式

---

## 相关

- 上一章：[../chapter-01-hello-world/](../chapter-01-hello-world/)
- 下一章：[../chapter-03-bootloader-display/](../chapter-03-bootloader-display/)
- 附录：[appendix-C EDK II 文件](../appendix-C-edk2-files/) · [appendix-B 获取 MikanOS](../appendix-B-get-mikanos/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md) · [../SETUP.md](../SETUP.md)
