## 5.4 关键名词 · 本章在链中的位置

> **§5 子笔记 4/5** · [§5 索引](./section-5-UEFI启动流程.md)

---

### 配套关键名词

| 名词 | 一句话 |
|------|--------|
| **UEFI** | 固件交互**标准**（≈ **HTTP 只定协议**）— 不提供程序；主板/OVMF 是实现 |
| **EDK II** | UEFI 规范的 **官方 C 完整实现套件** — 可编 **整板固件** 或 **独立 `.efi`** → [Ch2 §2.1](../../chapter-02-edk2-memmap/notes/section-2-1-EDK-II是什么与行业定位.md) |
| **FAT / FAT32** | 简单跨平台文件系统；UEFI 规范要求 **EFI 系统分区** 常用 FAT32，固件才能按路径打开 **`BOOTX64.EFI`** |
| **BOOTX64.EFI** | x86_64 **默认 UEFI 引导应用** — **功能 ≈ 传统 BIOS 的 IPL**，但是完整 PE 程序、可调固件 API；Ch1 用 **C + lld-link** 编译 |
| **EfiMain** | UEFI 应用的 **C 入口** — 等价于用户态的 `main`，参数为 `ImageHandle` + **`SystemTable`** |
| **OVMF + QEMU** | **OVMF** = EDK II 构建的开源 UEFI 固件；`-bios /usr/share/ovmf/OVMF.fd` + `fat:rw:esp` **模拟** 真实 UEFI 启动 |

→ OVMF 命令与排错 [SETUP.md](../../SETUP.md) · [§3 真机与 QEMU](./section-3-真机与QEMU测试.md)

---

### 本章程序在链中的位置

```
[ 硬件 ]
[ UEFI 固件 ]              ← 主板 ROM，非你编写
[ BOOTX64.EFI / Hello ]    ← 本章 — EfiMain + ConOut
[ MikanLoader / 内核 ]     ← Ch2+
```

**本章尚未涉及：** 分页、中断、自制文件系统 — 仅 **单 UEFI 应用** 调固件输出文本。

→ 七步流程 [5.1](./section-5-1-UEFI七步启动流程.md) · 全链路 [§7](./section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

← [5.3 四大区别](./section-5-3-BIOS与UEFI四大区别.md) · [§5 索引](./section-5-UEFI启动流程.md) · 下一篇 [5.5 Ch2 衔接 · 自检](./section-5-5-Ch2衔接与自检.md)
