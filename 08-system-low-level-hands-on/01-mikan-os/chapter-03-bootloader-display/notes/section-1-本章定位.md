## 1. 本章定位

> **《从零自制操作系统》Ch 3 屏幕显示实践和引导加载器**

---

### 一、全书分水岭

| 之前（Ch 1–2） | **本章起** |
|----------------|------------|
| 单一 **UEFI 应用**（MikanLoader） | **Loader + Kernel 分离** |
| 文本 Hello / memmap | **像素级** 屏幕控制 |
| 仍在 Boot Services 环境 | 内核开始 **脱离 UEFI 应用模型**（极简 `hlt` 循环） |

**本章问题：** 谁负责读盘、谁负责画图？硬件信息如何 **从固件时代交接到内核**？

→ **完整五步法（通读）：** [§0 MikanLoader + GOP → kernel main](./section-1-完整流程Miniload-GOP-kernel.md)

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **QEMU 监视器** | 寄存器、内存 — 底层调试 |
| **kernel.elf** | C++ 独立内核 · **ELF** · 入口 + `hlt` |
| **Loader 职责** | 读文件 · 分配页 · 解析 ELF · **跳转** |
| **GOP** | Graphics Output Protocol · **Frame Buffer** |
| **KernelMain** | 接收帧缓冲参数 · 内核绘图 |
| **EFI_STATUS** | 分配失败等 → 打印并 **安全停机** |
| **汇编** | `lea` / `mov` / `[]` ↔ 指针 |

---

### 三、架构图

```
┌─────────────────────────────────────┐
│  MikanLoader (UEFI .efi)            │
│  · GOP → **FrameBufferConfig**（基址+格式，非截图）│
│  · 读 kernel.elf · ELF 加载          │
│  · ExitBootServices → KernelMain(…)  │
└──────────────┬──────────────────────┘
               │ 跳转
               ▼
┌─────────────────────────────────────┐
│  kernel.elf (ELF)                   │
│  · KernelMain — 像素绘图             │
│  · 初期：hlt 死循环                  │
└─────────────────────────────────────┘
```

→ 衔接 [Ch2 §2.3 MikanLoader](../chapter-02-edk2-memmap/notes/section-2-3-MikanLoader是什么.md) · 全链路 [§0 完整流程](./section-1-完整流程Miniload-GOP-kernel.md)

---

← [Ch 3 导读](../README.md) · 下一节 [2. QEMU 与寄存器](./section-2-QEMU监视器与寄存器.md)
