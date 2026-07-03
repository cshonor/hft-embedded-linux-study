# 《从零自制操作系统》· 学习大纲

> **裁剪说明：** 🔴 精读 · 🟡 选读 · ⚪ 可跳过（HFT 时间紧时）  
> **笔记目录：** 各章独立目录 `chapter-XX-slug/`（导读 `README.md` + `notes/section-*.md`）— 对齐 [02 30 天 OS](../02-30days-os/) 的 `day-XX-slug/` 框架

| 章 | 中文标题 | 标签 | 笔记 |
|----|----------|------|------|
| **1** | 计算机工作原理和 Hello World | 🔴 | [chapter-01-hello-world](./chapter-01-hello-world/) |
| **2** | EDK II 和内存映射 | 🔴 | [chapter-02-edk2-memmap](./chapter-02-edk2-memmap/) |
| **3** | 屏幕显示实践和引导加载器 | 🟡 | [chapter-03-bootloader-display](./chapter-03-bootloader-display/) |
| **4** | 像素绘图和 make 入门 | ⚪ | [chapter-04-pixel-make](./chapter-04-pixel-make/) |
| **5** | 文本显示和控制台类 | ⚪ | [chapter-05-console-text](./chapter-05-console-text/) |
| **6** | 鼠标输入和 PCI | 🟡 | [chapter-06-mouse-pci](./chapter-06-mouse-pci/) |
| **7** | 中断和 FIFO | 🔴 | [chapter-07-interrupt-fifo](./chapter-07-interrupt-fifo/) |
| **8** | 内存管理 | 🔴 | [chapter-08-memory](./chapter-08-memory/) |
| **9** | 叠加过程 | ⚪ | [chapter-09-layers](./chapter-09-layers/) |
| **08** | 窗口 | ⚪ | [chapter-10-window](./chapter-10-window/) |
| **09** | 定时器和 ACPI | 🔴 | [chapter-11-timer-acpi](./chapter-11-timer-acpi/) |
| **10** | 键盘输入 | ⚪ | [chapter-12-keyboard](./chapter-12-keyboard/) |
| **11** | 多任务处理（1） | 🔴 | [chapter-13-multitask1](./chapter-13-multitask1/) |
| **12** | 多任务处理（2） | 🔴 | [chapter-14-multitask2](./chapter-14-multitask2/) |
| **15** | 终端 | ⚪ | [chapter-15-terminal](./chapter-15-terminal/) |
| **16** | 命令 | ⚪ | [chapter-16-commands](./chapter-16-commands/) |
| **17** | 文件系统 | 🟡 | [chapter-17-filesystem](./chapter-17-filesystem/) |
| **18** | 应用 | ⚪ | [chapter-18-apps](./chapter-18-apps/) |
| **19** | 分页 | 🔴 | [chapter-19-paging](./chapter-19-paging/) |
| **20** | 系统调用 | 🔴 | [chapter-20-syscall](./chapter-20-syscall/) |
| **21** | 窗口应用 | ⚪ | [chapter-21-window-apps](./chapter-21-window-apps/) |
| **22** | 图形和事件（1） | ⚪ | [chapter-22-graphics-events1](./chapter-22-graphics-events1/) |
| **23** | 图形和事件（2） | ⚪ | [chapter-23-graphics-events2](./chapter-23-graphics-events2/) |
| **24** | 多终端 | ⚪ | [chapter-24-multi-terminal](./chapter-24-multi-terminal/) |
| **25** | 使用应用读取文件 | 🟡 | [chapter-25-app-read-file](./chapter-25-app-read-file/) |
| **26** | 使用应用写入文件 | 🟡 | [chapter-26-app-write-file](./chapter-26-app-write-file/) |
| **27** | 应用的内存管理 | 🔴 | [chapter-27-app-memory](./chapter-27-app-memory/) |
| **28** | 日文显示和重定向 | ⚪ | [chapter-28-japanese-redirect](./chapter-28-japanese-redirect/) |
| **29** | 应用间通信 | 🟡 | [chapter-29-ipc](./chapter-29-ipc/) |
| **30** | 额外应用 | ⚪ | [chapter-30-extra-apps](./chapter-30-extra-apps/) |
| **31** | 前方的路 | 🟡 | [chapter-31-road-ahead](./chapter-31-road-ahead/) |

## 附录 A–F

| 附录 | 中文标题 | 笔记 |
|------|----------|------|
| A | 配置开发环境 | [appendix-A-dev-env](./appendix-A-dev-env/) → [SETUP.md](./SETUP.md) |
| B | 获取 MikanOS | [appendix-B-get-mikanos](./appendix-B-get-mikanos/) |
| C | EDK II 文件说明 | [appendix-C-edk2-files](./appendix-C-edk2-files/) |
| D | C++ 中的模板 | [appendix-D-cpp-templates](./appendix-D-cpp-templates/) |
| E | iPXE | [appendix-E-ipxe](./appendix-E-ipxe/) |
| F | ASCII 码表 | [appendix-F-ascii-table](./appendix-F-ascii-table/) |

---

## Ch 1–2 要点速览

- **UEFI** 启动链 · **EDK II** · **EFI 内存 map**
- **Ch 1 Hello World：** 二进制 `BOOTX64.EFI` → Secure Boot / QEMU → **EfiMain + PE/COFF**
- **Ch 2 EDK II / memmap：** **MikanLoader** · `GetMemoryMap()` · **memmap CSV** · 指针 `->` / `**`
- 与 [01 Day 1](../02-30days-os/day-01-boot-asm/) 对照：BIOS `0x7C00` vs UEFI **`/EFI/BOOT/BOOTX64.EFI`**

→ 详读 [chapter-01-hello-world/](./chapter-01-hello-world/) · [chapter-02-edk2-memmap/](./chapter-02-edk2-memmap/)

---

## Ch 7–8 · 13–14 要点速览

- **APIC** 中断 · **FIFO** · 物理/线性 **内存管理**
- **协作/抢占多任务** — 对照 [01 Day 15–16](../02-30days-os/day-15-multitask1/)

→ 详读 [chapter-07-interrupt-fifo/](./chapter-07-interrupt-fifo/) · [chapter-08-memory/](./chapter-08-memory/) · [chapter-13-multitask1/](./chapter-13-multitask1/) · [chapter-14-multitask2/](./chapter-14-multitask2/)

---

## Ch 19–20 要点速览

- **页表** · 用户/内核地址空间
- **系统调用** — 对照 [01-CSAPP Ch9](../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [04-Linux-Kernel-Development](../../04-Linux-Kernel-Development/) · [07-The-Linux-Programming-Interface](../../07-The-Linux-Programming-Interface/)

→ 详读 [chapter-19-paging/](./chapter-19-paging/) · [chapter-20-syscall/](./chapter-20-syscall/)

---

**笔记进度：** 目录骨架已建（Ch 0–31 + 附录 A–F）；正文随学习在 `chapter-XX/notes/` 增量补充。

→ 学习计划 [LEARNING_PLAN.md](./LEARNING_PLAN.md) · 环境 [SETUP.md](./SETUP.md)
