## 3.4 地址清单 · UEFI 内存类型

> **§3 子笔记 4/5** · [§3 索引](./section-3-主存储器与内存映射.md)

---

### 两层理解（建议顺序）

#### 第一层：EFI 内存类型 = 地址标签

UEFI 固件扫描 **全部物理地址空间**（**内存条 RAM** + **MMIO 硬件映射**），把整片空间切成很多 **连续段**，给 **每一段** 贴 **一个 Type 标签**。

| ❌ 误区 | ✅ 正解 |
|---------|---------|
| 单独多出一块「EFI 专属内存」 | **整块物理地址** 全部被固件 **分类标记** |
| 只在内存条里划出 N 块固定区域 | **RAM 与 MMIO 都会分段、都会出现在 map 里** |
| 低地址=固件、高地址=程序 | **地址高低无绝对规则** — 看 **每段的标签** |
| Runtime 属于 Reserved 或 Boot 的子类 | **全部 Type 平级** — 见下节 |

`GetMemoryMap()` 返回的就是这张 **完整清单**：**每一段起始地址 + 长度 + Type + Attribute**。

#### 第二层：完整常见枚举（全部平级）

**所有这些名字都在同一套 `EFI_MEMORY_TYPE` 枚举里** —— 固件给 **每一段** 只贴 **其中一个** 标签，**不存在谁包含谁**：

| # | Type（独立选项） | 一句话 |
|---|------------------|--------|
| 1 | **EfiConventionalMemory** | 空闲内存条 RAM |
| 2 | **EfiLoaderCode / EfiLoaderData** | 你的 `bootx64.efi` / MikanLoader |
| 3 | **EfiACPIReclaimMemory** | ACPI 电源/硬件配置表 |
| 4 | **EfiRuntimeServicesCode / EfiRuntimeServicesData** | **Runtime 固件** — 关机、读时间、NVRAM 变量等 **永久后台** |
| 5 | **EfiBootServicesCode / EfiBootServicesData** | **Boot 期固件** — GetMemoryMap、分配 Boot 内存等 **临时工具** |
| 6 | **EfiReservedMemoryType** | 固件底层私有保留 — **连 Runtime 接口都不对外** |
| 7 | **EfiMemoryMappedIO / EfiMemoryMappedIOPortSpace** 等 | 显卡显存、外设寄存器 — **不是普通 RAM** |

**大街比喻：** 整片街道分成很多门面 → UEFI 给每个门面 **贴一张标签** → OS **默认只租用 Conventional（商铺）**。

---

### 平级枚举 vs 「四层直觉」（打通两份笔记）

| 视角 | 在哪篇 | 干什么用 |
|------|--------|----------|
| **四层占用图** | [3.2](./section-3-2-RAM四层占用.md) | **空间直觉** — 固件 / MMIO / Loader / 空闲 **大致** 叠在哪 |
| **管家 / 工作台比喻** | [3.3](./section-3-3-固件与EFI应用内存隔离.md) | **谁在用** — 固件 vs 你的 `.efi` vs 未来内核 |
| **Type 枚举 + CSV** | **本篇 3.4** | **精确标签** — `GetMemoryMap()` 每一行的 **Type 字段** |

**不是两套独立分类。** 四层图里的「① 固件区」在 CSV 里可能对应 **Runtime / Boot / ACPI** 等 **多条平级 Type**；`RuntimeServicesCode` **本身就是枚举里单独一条**，不属于 Reserved，也不属于 BootServices 或 Loader。

---

### Runtime / Boot / Reserved 三分法（易混点）

| Type | 归属 | 生命周期 | 和另两类差在哪 |
|------|------|----------|----------------|
| **EfiBootServicesCode/Data** | UEFI **Boot 期**固件 | 仅引导阶段 | **`ExitBootServices()` 后** 在新 map 里 **通常** 变可回收 — 须 **重读 map** |
| **EfiRuntimeServicesCode/Data** | UEFI **Runtime**固件 | 开机 → 关机 **全程** | OS 运行期间 **必须保留** — 读时间、变量、关机等仍走固件 |
| **EfiReservedMemoryType** | 固件 **底层私有** | 全程锁定 | **比 Runtime 更封闭** — 无对外固件接口，内核 **完全不能碰** |

**`EfiRuntimeServicesCode` 答你的核心问题：** 它 **就是独立的一类**，和 BootService、Loader、Reserved **全部平级** —— 不是 Reserved 的子集，也不是 Boot 的一部分。

---

### 生命周期总表（一目了然）

| Type 标签 | 归属主体 | 生命周期 | 内核能否回收复用 |
|-----------|----------|----------|------------------|
| **EfiConventionalMemory** | 空闲内存条 | 永久空闲 | ✅ **完全可以** — 分配池 **唯一默认来源** |
| **EfiLoaderCode/Data** | 你的 `bootx64.efi` | 仅引导阶段 | ✅ **`ExitBootServices` 后** 重读 map，**通常** 可回收 |
| **EfiBootServicesCode/Data** | UEFI Boot 工具 | 仅引导阶段 | ✅ 同上 — Exit 后 **通常** 变 Conventional |
| **EfiRuntimeServicesCode/Data** | UEFI Runtime 后台 | 开机 → 关机全程 | ❌ **禁止覆盖** — 必须永久保留 |
| **EfiACPIReclaimMemory** | ACPI 配置表 | 引导阶段锁定 | ✅ 内核 **读完 ACPI、初始化完硬件后** 或可回收 |
| **EfiReservedMemoryType** | 固件私有资源 | 全程锁定 | ❌ **完全不能碰** |
| **MMIO 系列** | 显卡 / 外设寄存器 | 全程锁定 | ❌ **不是 RAM** — 禁止当堆分配 |

---

### 开发铁则（写 OS 必须遵守）

初始化 **自有物理内存池 / 分配器** 时：

> **默认只把 `EfiConventionalMemory` 纳入可分配范围。**

其余 Type **一律不能当堆随便读写、不能随便划进分配池**（ACPI 等 **晚于 Ch8** 再谈规范回收）。

**Ch2 MikanLoader：** 先 **只读 map、导出 CSV** — 建立「物理世界账本」。

→ 四层直觉：[3.2](./section-3-2-RAM四层占用.md) · 权限比喻：[3.3](./section-3-3-固件与EFI应用内存隔离.md)

---

### `GetMemoryMap()` 返回什么

调用 **`GetMemoryMap()`**（Boot Services 期）→ **`EFI_MEMORY_DESCRIPTOR` 数组**：

| 字段 | 含义 |
|------|------|
| 起始物理地址 | 这段从哪开始 |
| 长度 | 多长 |
| **Type** | 上表中的 **某一个平级枚举值** |
| **Attribute** | 可执行 / 可写 / 缓存策略等 |

MikanLoader 导出 **memmap CSV** → [§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)

```
不查表就把内核加载到 0x100000：
  → 可能盖住 Runtime / ACPI
  → 可能写到 MMIO（当 RAM 写 = 改寄存器）
  → QEMU 直接挂
```

---

### 内存属性位（Type 之外还有 Permission）

**Type** = 用途标签；**Attribute** = 读 / 写 / 执行权限。

| 属性 | 白话 |
|------|------|
| **可执行（XP 相关）** | **LoaderCode** 的 `.text` 要跑机器码 |
| **不可写** | LoaderCode 常 **可执行、不可写** — 防 `memset` 篡改正在跑的代码 |
| **EFI_MEMORY_WB** 等 | 缓存策略 — Ch 8+ 再细 |

---

### 四条总结

1. **全部 Type 平级** — 一段内存 **只有一个** 标签；Runtime ≠ Reserved 的子类。
2. **分配池默认只认 Conventional** — 其余 Type 特殊处理。
3. **Boot + Loader 是临时的** — **`ExitBootServices` + 重读 map** 后再谈回收。
4. **Runtime + Reserved + MMIO 全程锁死** — 不能当堆；Reserved 比 Runtime **更不可碰**。

---

### 口述版 · 一次性理清（背这个）

**第一层 — EFI 内存类型是什么？**

UEFI 开机扫描 **整片物理地址**（**内存条 RAM** + **MMIO 硬件地址**），切成很多连续段，给 **每段贴一个 Type 标签**。  
**不是** 单独划出一块「EFI 专属内存」，也 **不是** 在内存条里固定划出 N 块区。

**第二层 — 常见 Type 全部平级**

`Conventional` · `LoaderCode/Data` · `ACPIReclaim` · `RuntimeServicesCode/Data` · `BootServicesCode/Data` · `Reserved` · `MMIO` —— **同一套枚举，互不包含**。  
`RuntimeServicesCode` **就是单独一条**，不属于 Boot、Loader 或 Reserved。

**第三层 — 和 §3.2 四层图什么关系？**

| 你看到的 | 本质 |
|----------|------|
| [3.2 四层图](./section-3-2-RAM四层占用.md) | **空间直觉** — 固件 / MMIO / Loader / 空闲 **大致** 在哪 |
| [3.3 管家比喻](./section-3-3-固件与EFI应用内存隔离.md) | **谁在用** |
| **本篇 Type 表 + CSV** | **精确标签** — `GetMemoryMap()` 每行的 **Type 字段** |

**不是两套分类。** 四层里的「① 固件区」在 CSV 里会拆成 **Runtime / Boot / ACPI** 等多条 **平级 Type**。

**一句话：** 内核分配池 **默认只从 Conventional 划**；Boot/Loader **Exit 后或可回收**；Runtime / Reserved / MMIO **全程别当堆**。

---

← [3.3 固件 vs EFI 应用](./section-3-3-固件与EFI应用内存隔离.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一篇 [3.5 与 mmap 区别 · 自检](./section-3-5-与mmap区别与自检.md)
