## 3.4 地址清单 · UEFI 内存类型

> **§3 子笔记 4/5** · [§3 索引](./section-3-主存储器与内存映射.md)

---

### 内存映射的核心作用 —— 「地址清单」

**内存映射 = 给内核的一张明确清单：**

| 每条记录告诉你 | 例子 |
|----------------|------|
| **从哪开始** | 起始物理地址 |
| **多长** | 字节长度 |
| **什么类型** | Conventional / LoaderCode / ACPI / Reserved … |
| **什么属性** | 可执行？可写？可缓存？ |

```
你若不查表，直接把内核加载到 0x100000：
  → 可能盖住 UEFI 数据
  → 可能写到 MMIO（当成 RAM 写 = 乱改设备寄存器）
  → 结果：黑屏、三重故障、QEMU 直接挂
```

**OS 开发原则：** 只把 **`EfiConventionalMemory`**（及规范明确允许的类型）纳入 **自有的物理页池**。

→ 四层直觉背景：[3.2 RAM 四层占用](./section-3-2-RAM四层占用.md)

---

### UEFI 内存类型与属性（对照 CSV）

`GetMemoryMap()` 返回 **`EFI_MEMORY_DESCRIPTOR`** 数组 —— 每一段连续物理区域一行。

| 类型（常见） | 对应哪层 | 含义 |
|--------------|----------|------|
| **EfiConventionalMemory** | ④ | **可用常规内存** — 分配器主目标 |
| **EfiLoaderCode** | ③ | 当前 **UEFI 加载器代码段**（如 MikanLoader 的 `.text`） |
| **EfiLoaderData** | ③ | Loader 的 **数据 / BSS** |
| **EfiACPIReclaimMemory** | ① | **ACPI 表** — 启动后可回收，但别在 Loader 期乱踩 |
| **EfiReservedMemoryType** | ② 等 | **保留** — 含固件、特殊用途 |
| **MMIO 相关类型** | ② | **内存映射 I/O** — **不是普通 RAM** |

**属性位（示意）— 和「能不能写」直接相关：**

| 属性 | 白话 |
|------|------|
| **EFI_MEMORY_XP**（可执行） | 这段可以 **当代码跑** |
| **不可写** | **LoaderCode** 常是 **可执行、不可写** — 防止被当成普通数组乱改，破坏正在跑的 EFI 程序 |
| **EFI_MEMORY_WB** 等 | 缓存策略 — 影响性能与一致性，Ch 8+ 再细 |

**例子：** CSV 里一行标 **`LoaderCode`** 的区间 = **你之前加载的 EFI 程序代码** —— **能执行，别当堆去 `memset`**。

→ 导出流程：[§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)

---

← [3.3 固件 vs EFI 应用](./section-3-3-固件与EFI应用内存隔离.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一篇 [3.5 与 mmap 区别 · 自检](./section-3-5-与mmap区别与自检.md)
