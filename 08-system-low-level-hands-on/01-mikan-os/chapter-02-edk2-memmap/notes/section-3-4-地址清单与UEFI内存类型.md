## 3.4 地址清单 · UEFI 内存类型

> **§3 子笔记 4/5** · [§3 索引](./section-3-主存储器与内存映射.md)

---

### 核心结论（先看懂）

**纠正一个最常见误区：** 物理内存 **不是** 简单「低地址全是固件、高地址全是你的程序」。

UEFI 按 **「内存类型」** 分片管理 —— **地址高低没有绝对划分**；每一段在 Memory Map 里有 **固定用途、回收规则、访问权限**。  
→ 四层直觉图：[3.2 RAM 四层占用](./section-3-2-RAM四层占用.md) · 权限比喻：[3.3 固件 vs EFI 应用](./section-3-3-固件与EFI应用内存隔离.md)

---

### 五类内存 · 逐行解读（MikanOS 口述版）

| 类型 | 归属（四层） | 干什么 | 回收 / 权限 |
|------|--------------|--------|-------------|
| **EfiRuntimeServicesCode / Data** | ① 固件 | OS 跑起来后仍要用的 **Runtime 服务**（读时间、NVRAM 变量等） | ❌ **OS 运行期间永久保留** — 内核 **不得当堆用、不得覆盖** |
| **EfiBootServicesCode / Data** | ① 固件 | **仅 Boot 阶段** 的固件服务（分配内存、装协议、GetMemoryMap…） | ⚠️ Boot 期间固件管；**`ExitBootServices()` 后** 在 **新 map** 里 **通常** 变为可用 — 须 **Exit 后再读一次 map**，不是「不用读表就全变空闲」 |
| **EfiLoaderCode / LoaderData** | ③ EFI 应用 | 你的 **`BOOTX64.EFI` / MikanLoader** 加载进 RAM 的代码与数据 | ⚠️ **正在跑 Loader 时别覆盖**；**ExitBootServices 后** 一般 **可回收** 给内核 |
| **EfiConventionalMemory** | ④ 可用 RAM | **纯空闲** — 未被固件/硬件占用的常规内存 | ✅ 内核、栈、堆、页表的 **主来源** |
| **MMIO / Reserved** | ② 硬件保留 | **显存 FrameBuffer**、PCI BAR、设备 **寄存器映射** | ❌ **不是 RAM** — 当内存读写会 **黑屏 / 三重故障** |

**四条一句总结（结合前面疑问）：**

1. **Runtime 固件区** — 永久锁给固件，**任何程序都不能占**。  
2. **Boot 固件区 + 你的 `.efi` 区** — **临时**；内核交接后 **按新 map 回收**（Loader 先 **ExitBootServices**）。  
3. **MMIO / 显存** — **永久硬件保留**，全程 **不能** 当程序堆栈。  
4. **ConventionalMemory** — **原生空闲**，系统 **只应从这里划** 自有物理页池。

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
