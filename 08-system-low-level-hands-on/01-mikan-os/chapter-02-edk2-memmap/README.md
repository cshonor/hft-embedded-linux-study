# Ch 2 · EDK II 和内存映射

> **原书第 2 章** · HFT **🔴** · 官方源码标签 `osbook_day02`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **规范化开发起点：** EDK II 框架 + **物理内存摸底**（Memory Map）

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
| 2. EDK II 与 MikanLoader | [notes/section-2-EDK-II与MikanLoader.md](./notes/section-2-EDK-II与MikanLoader.md) · [Ch1 §7 两阶段全链路](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) |
| 3. 主存储器与内存映射 | [notes/section-3-主存储器与内存映射.md](./notes/section-3-主存储器与内存映射.md) — **物理 RAM 四层** · 非动态库 mmap · LoaderCode |
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

- [ ] 说清 **EDK II** 的用途及 `<Uefi.h>` 带来的便利
- [ ] 解释 **内存映射** — 哪些物理地址段空闲、哪些已被 UEFI 占用
- [ ] 描述 **`gBS->GetMemoryMap()`** 与 **memmap CSV** 导出流程
- [ ] 会用 **`->`** 访问结构体成员，理解 UEFI 中 **指针的指针** 常见模式

---

## 相关

- 上一章：[../chapter-01-hello-world/](../chapter-01-hello-world/)
- 下一章：[../chapter-03-bootloader-display/](../chapter-03-bootloader-display/)
- 附录：[appendix-C EDK II 文件](../appendix-C-edk2-files/) · [appendix-B 获取 MikanOS](../appendix-B-get-mikanos/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md) · [../SETUP.md](../SETUP.md)
