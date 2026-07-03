## 1. 本章定位

> **《从零自制操作系统》Ch 2 EDK II 和内存映射**

---

### 一、与 Ch 1 的衔接

| Ch 1 | **Ch 2** |
|------|----------|
| 手写 / 简易 C **Hello World** | **EDK II 库** 重写 — 代码更简洁 |
| 程序无名或实验性质 | 正式命名 **MikanLoader** — 引导加载器 **雏形** |
| 仅输出文本 | **摸底物理内存** — 导出 **memmap** 供后续 OS 使用 |

**本章问题：** OS 尚未运行 → 如何知道 **哪些 RAM 可用、哪些被固件占用**？

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **EDK II** | 开源 UEFI **参考实现 + 工具链** — BIOS / Loader / OVMF；详见 [§2 导读 · 2.1](./section-2-1-EDK-II是什么与行业定位.md) |
| **MikanLoader** | Ch1 Hello World 的 EDK II 版 + 内存导出 |
| **Memory Map** | 物理地址段的 **类型与用途** 一览表 |
| **Boot Services** | `gBS->GetMemoryMap()` |
| **文件 I/O** | `EFI_FILE_PROTOCOL` → U 盘 **memmap** CSV |
| **指针** | `->`、双重指针 — UEFI 接口必备 |

---

### 三、在全书中的位置

```
Ch 1  UEFI 应用能跑
    ↓
Ch 2  知道物理内存布局（memmap）  ← 本章
    ↓
Ch 3+ 显示 · 内核 · 页表 · 分配器…
```

**HFT 读法：** 物理内存布局是 **NUMA / 大页 / 绑核** 的更底层前提 — 与 [03 SysPerf](../../../15-Systems-Performance-2nd/) 硬件拓扑对照时有直觉（选读）。

---

← [Ch 2 导读](../README.md) · 下一节 [2. EDK II · 2.1](./section-2-1-EDK-II是什么与行业定位.md)
