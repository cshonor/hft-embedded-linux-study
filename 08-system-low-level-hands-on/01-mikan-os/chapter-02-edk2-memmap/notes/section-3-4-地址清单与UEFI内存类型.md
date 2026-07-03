## 3.4 地址清单 · UEFI 内存类型

> **§3 子笔记 4/5** · [§3 索引](./section-3-主存储器与内存映射.md)

---

### 两层理解（建议顺序）

#### 第一层：EFI 内存类型 = 地址标签

UEFI 固件扫描 **全部物理地址空间**（**内存条 RAM** + **MMIO 硬件映射**），把整片空间切成很多 **连续段**，给 **每一段** 贴一个 **Type 标签**。

| ❌ 误区 | ✅ 正解 |
|---------|---------|
| 单独多出一块「EFI 专属内存」 | **整块物理地址** 全部被固件 **分类标记** |
| 只在内存条里划出 N 块固定区域 | **RAM 与 MMIO 都会分段、都会出现在 map 里** |
| 低地址=固件、高地址=程序 | **地址高低无绝对规则** — 看 **每段的标签** |

`GetMemoryMap()` 返回的就是这张 **完整清单**：**每一段起始地址 + 长度 + Type + Attribute**。

#### 第二层：常见六类标签（+ Boot 期一类）

口述常归纳 **六类**；CSV 里还会见到 **Boot Services**（Boot 期固件，Exit 后或变 Conventional）：

| # | 标签（Type） | 大街比喻 | 内核分配池 |
|---|--------------|----------|------------|
| 1 | **EfiConventionalMemory** | **商铺** — 空闲可租 | ✅ 默认唯一主来源 |
| 2 | **EfiLoaderCode / Data** | **临时工地** — 你的 `bootx64.efi` | ❌ Loader 跑着勿占；Exit 后或可回收 |
| 3 | **EfiACPIReclaimMemory** | **档案室** — ACPI 硬件配置表 | ⚠️ Loader 期勿踩；内核后或可回收 |
| 4 | **EfiRuntimeServicesCode / Data** | **政府永久办公楼** — Runtime 固件 | ❌ OS 运行期间永久保留 |
| 5 | **EfiReservedMemoryType** | **封条门面** — 平台/固件永久保留 | ❌ 全程禁止当堆 |
| 6 | **MMIO 系列** | **设备机房** — 显存、外设寄存器 | ❌ **不是 RAM** — 读写=控硬件 |
| + | **EfiBootServicesCode / Data** | **Boot 期物业办公室** — 仅 Boot 服务 | ❌ Boot 期勿碰；**Exit 后重读 map** |

**大街比喻（整段）：** 整片街道分成很多门面 → UEFI 给每个门面贴标签 → 你的 OS **只能租用「商铺」（Conventional）**；政府楼、机房、临时工地、封条门面都不能当商铺随便用。

---

### 开发铁则（写 OS 必须遵守）

初始化 **自有物理内存池 / 分配器** 时：

> **默认只把 `EfiConventionalMemory`（普通空闲 RAM）纳入可分配范围。**

其余类型 **一律不能当堆随便读写、不能随便划进分配池**：

| 不能纳入分配池 | 原因 |
|----------------|------|
| **Runtime 固件区** | OS 运行期间固件仍要 |
| **Boot Services 固件区** | Boot 期由固件管；Exit 前勿覆盖 |
| **LoaderCode / LoaderData** | 你的 `.efi` 正在跑 — Exit 前勿覆盖 |
| **MMIO / 显存 / 设备寄存器** | **不是 RAM** — 写进去是 **控硬件** |
| **Reserved 等保留区** | 固件/平台特殊用途 — 全程禁止当堆 |

**Ch2 MikanLoader 阶段：** 先 **只读 map、导出 CSV** — 建立「物理世界账本」；Ch8+ 再在 **Conventional** 里做分配器。

→ 四层直觉：[3.2](./section-3-2-RAM四层占用.md) · 权限比喻：[3.3](./section-3-3-固件与EFI应用内存隔离.md)

---

### 误区纠正（再强调一次）

物理内存 **不是**「低地址全是固件、高地址全是程序」，也 **不是**「内存条里单独划出 6 块 EFI 区」。  
UEFI 对 **RAM + MMIO 整片地址** 分段贴标签 —— **地址高低无绝对划分**；每段有 **用途、回收规则、属性**。

---

### `GetMemoryMap()` 返回什么

调用固件 API **`GetMemoryMap()`**（Boot Services 期）→ 一整块 **`EFI_MEMORY_DESCRIPTOR` 数组**：

**每一行 = 一段连续物理内存**，至少包含：

| 字段 | 含义 |
|------|------|
| 起始物理地址 | 这段从哪开始 |
| 长度（页/字节） | 多长 |
| **Type** | 下面表格中的类型 |
| **Attribute** | 可执行 / 可写 / 缓存策略等 |

MikanLoader 导出 **memmap CSV** 就是把这张表变成人眼能看的版本 → [§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)

---

### 内存类型对照（CSV 里常见行）

| # | 类型 | 四层 | 含义 | 能否进内核分配池 |
|---|------|------|------|------------------|
| 1 | **EfiConventionalMemory** | ④ 可用 RAM | **纯净空闲** — 无固件/硬件占用 | ✅ **唯一合法主来源** |
| 2 | **EfiLoaderCode / LoaderData** | ③ EFI 应用 | 你的 **`BOOTX64.EFI` / MikanLoader** 代码段、数据段 | ❌ Loader 跑着时勿覆盖；**ExitBootServices 后** 常可回收 |
| 3 | **EfiBootServicesCode / Data** | ① 固件 | **仅 Boot 阶段** 固件服务用 | ❌ Boot 期勿碰；Exit 后 **读新 map** 再看是否变 Conventional |
| 4 | **EfiRuntimeServicesCode / Data** | ① 固件 | **Runtime 服务**（时间、NVRAM 等） | ❌ **OS 运行期间永久保留** |
| 5 | **EfiACPIReclaimMemory** | ① 固件相关 | **ACPI 表**（电源管理等） | ⚠️ **Loader 阶段勿乱踩**；内核启动后 **可能** 回收（晚于 Ch8 再细） |
| 6 | **EfiReservedMemoryType** | ② 保留 | 固件/平台 **永久或特殊** 占用 | ❌ 全程禁止当堆 |
| 7 | **MMIO 相关类型** | ② 硬件保留 | **FrameBuffer 显存**、外设 **寄存器映射** | ❌ **绝不是 RAM** — 乱读写 → 黑屏 / 三重故障 |

```
不查表就把内核加载到 0x100000：
  → 可能盖住 UEFI / ACPI
  → 可能写到 MMIO（当 RAM 写 = 改寄存器）
  → QEMU 直接挂
```

---

### 内存属性位（Type 之外还有 Permission）

**类型** 说「这片是干什么用的」；**属性** 说「能不能读/写/执行」—— 和 **代码段 / 数据段** 权限直接相关。

| 属性 | 白话 |
|------|------|
| **EFI_MEMORY_XP**（Execute Protect 相关位） | 标记 **可执行** — 如 **LoaderCode** 代码段要跑机器码 |
| **不可写** | **LoaderCode** 常配成 **可执行、不可写** — 防运行中代码被 `memset` 篡改 |
| **EFI_MEMORY_WB** 等 | **缓存策略** — 影响性能与一致性，Ch 8+ 再细 |

**例子：** CSV 里 **LoaderCode** 一行 = 你的 EFI 程序 **`.text`** —— **能执行，别当普通可写数组乱改**。

---

### 四条总结（口述巩固）

1. **真正能给内核随便划池的，默认只有 `EfiConventionalMemory`。**
2. **LoaderCode/Data**（你的 `.efi`）是 **临时占用** — **`ExitBootServices` 后** 按 **新 map** 往往可回收。
3. **MMIO、Reserved、Runtime 固件区** 全程 **锁死**，不能纳入分配池。
4. 内存除 **「类型 Type」** 外，还有独立 **「属性 Attribute」** — 区分代码段/数据段的 **读 · 写 · 执行**。

---

← [3.3 固件 vs EFI 应用](./section-3-3-固件与EFI应用内存隔离.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一篇 [3.5 与 mmap 区别 · 自检](./section-3-5-与mmap区别与自检.md)
