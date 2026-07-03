## 5.4 关键名词 · 本章在链中的位置

> **§5 子笔记 4/5** · [§5 索引](./section-5-UEFI启动流程.md)

---

### 配套关键名词

| 名词 | 一句话 |
|------|--------|
| **UEFI** | Unified Extensible Firmware Interface — **替代老旧 BIOS** 的可扩展固件；固化在主板 Flash，提供 **Boot/Runtime Services** 等标准接口 |
| **FAT / FAT32** | 简单跨平台文件系统；UEFI 规范要求 **EFI 系统分区** 常用 FAT32，固件才能按路径打开 **`BOOTX64.EFI`** |
| **BOOTX64.EFI** | x86_64 **默认 UEFI 引导应用** 文件名；你用 **C + lld-link** 编译的 **PE 产物**，放对路径才被加载 |
| **EfiMain** | UEFI 应用的 **C 入口** — 等价于用户态的 `main`，参数为 `ImageHandle` + **`SystemTable`** |
| **OVMF + QEMU** | **OVMF** = QEMU 用的开源 UEFI 固件；`-bios /usr/share/ovmf/OVMF.fd` + `fat:rw:esp` **模拟** 真实 UEFI 启动 |

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
