# Ch 1 §5 UEFI 启动流程 · BIOS 对比（索引）

> **MikanOS** · 原书第 1 章 · **🔴**  
> 本章 §5 拆成 **5 篇子笔记**，按顺序读。

**核心图：** 从通电到 `EfiMain` 跑起来 — MikanOS 走 **UEFI 线**，不是 BIOS 512 字节扇区线。

**逻辑框架（先建立再读子笔记）：** **UEFI ≈ HTTP**（只定规则）→ **EDK II** = 官方 C 完整实现（可编固件 + `.efi`）→ **`BOOTX64.EFI` 功能 ≈ 传统 IPL**（形态是完整 PE 程序）。详 [Ch2 §2.1](../../chapter-02-edk2-memmap/notes/section-2-1-EDK-II是什么与行业定位.md) · IPL 对照 [§1.四](./section-1-本章定位.md#ipl-与-bootx64efi功能等价形态不同)

---

## 阅读顺序

| # | 笔记 | 带走什么 |
|---|------|----------|
| **5.1** | [UEFI 七步启动流程](./section-5-1-UEFI七步启动流程.md) | 通电 → FAT 找 `BOOTX64.EFI` → **EfiMain** |
| **5.2** | [BIOS 传统启动流程](./section-5-2-BIOS传统启动流程.md) | 512B IPL · 0xAA55 · 无文件系统 |
| **5.3** | [BIOS 与 UEFI 四大区别](./section-5-3-BIOS与UEFI四大区别.md) | 长模式 / FAT 路径 / C 开发门槛 |
| **5.4** | [关键名词 · 本章在链中的位置](./section-5-4-关键名词与本章位置.md) | UEFI · FAT · EfiMain · OVMF |
| **5.5** | [Ch2 内存衔接 · 自检](./section-5-5-Ch2衔接与自检.md) | 管家/工作台 → Ch2 §3.3 · 口述巩固 |

**建议路径：** 5.1（主线）→ 5.2–5.3（对比）→ 5.4 → 5.5 → [§6 C 与文件格式](./section-6-C语言过渡与文件格式.md)

---

← [4. 结构与编码](./section-4-计算机结构与编码.md) · 开始 [5.1](./section-5-1-UEFI七步启动流程.md) · [§1 两条线速览](./section-1-本章定位.md#四核心区分bootx64efi--软盘启动两条线)
