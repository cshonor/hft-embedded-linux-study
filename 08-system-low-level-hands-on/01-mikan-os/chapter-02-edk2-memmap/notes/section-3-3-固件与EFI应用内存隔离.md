## 3.3 固件 vs 已加载 EFI 应用 · 内存隔离

> **§3 子笔记 3/5** · [§3 索引](./section-3-主存储器与内存映射.md)

> **直觉版（先记住）：** UEFI 固件是 **「底层管家」**；你编译的 **`BOOTX64.EFI`**（Ch1 Hello / Ch2 **MikanLoader**）是管家 **「请来的临时帮手」** —— 二者在内存里是 **两块完全隔离的区域**，不是混在一起的一坨代码。

---

### 1. 比喻对照

| 比喻 | 对应什么 | 说明 |
|------|----------|------|
| **管家的办公区** | **UEFI 固件** 占用的内存 | 固件核心服务、硬件初始化代码、**ACPI 表**、**Memory Map** 等 **系统级数据**；权限最高，**你的 Loader 不能直接乱写** |
| **客厅临时工作台** | **已加载的 EFI 应用**（`.efi`） | 固件把 `BOOTX64.EFI` **整份 PE 镜像** 载入 RAM —— 代码/数据在 **单独一段**（[3.2](./section-3-2-RAM四层占用.md) **③ LoaderCode / LoaderData**） |
| **你自己的办公室** | **MikanOS 内核**（Ch3+） | Loader 把 **kernel.elf** 加载进 **④ 可用 RAM**，长模式页表建好后正式运行 |
| **拆掉工作台、收回客厅** | **`ExitBootServices` 之后** | Boot 阶段占用的 **Loader / Boot Services 内存** 可在 map 里标成可用，内核 **回收复用** |

| 阶段 | 「帮手」在干什么 |
|------|----------------|
| **Ch1 Hello World** | 只在 **工作台** 上 `ConOut` 打印 —— 尚未读 map、未加载内核、未 **ExitBootServices** |
| **Ch2 MikanLoader** | **`GetMemoryMap`** → 导出 CSV → 加载 **kernel.elf** → **ExitBootServices** → 完整「临时工 → 内核」交接 |

---

### 2. 技术版：不是「低地址 = 固件、后面 = 应用」

物理地址 **不是** 简单从 0 起「固件占前半、应用占后半」。固件用 **Memory Map** 按 **类型** 标记 **每一段** RAM（详 [3.4](./section-3-4-地址清单与UEFI内存类型.md)）：

| 类型（常见） | 对应四层 | 谁占 | 内核能随便写吗？ |
|--------------|----------|------|------------------|
| **EfiRuntimeServicesCode/Data** | ① 固件 | OS 运行后 **仍保留** 的固件部分 | ❌ **长期勿动** |
| **EfiBootServicesCode/Data** | ① 固件 | 仅 **Boot 阶段** 的固件服务 | ❌ Boot 期间固件管；**ExitBootServices 后** 常可回收 |
| **EfiLoaderCode / LoaderData** | ③ EFI 应用 | **已加载的 `.efi`** | ⚠️ **正在跑** 时别覆盖；交接后可回收 |
| **EfiConventionalMemory** | ④ 可用 RAM | 空闲 | ✅ 内核、栈、堆 **主要从这里划** |
| **MMIO / Reserved** | ② 硬件保留 | 设备寄存器、显存 | ❌ **不是普通 RAM** |

---

### 3. Boot Services 期间的「权限直觉」

- 你的 **`EfiMain` / MikanLoader`** **不能** 直接读写 **管家办公区** —— 只能经 **`SystemTable` → `gBS` / 协议**（如 `ConOut`、`GetMemoryMap`）**间接** 调固件。
- 固件 **定布局、管硬件**；EFI 应用 **只在 Loader 区 + 固件允许的 API** 里活动。
- 这就是为什么 Ch1 手写 **`SystemTable->ConOut->OutputString`** 就够了 —— 你 **借管家的接口** 干活，而不是自己 mmap 固件区。

---

### 3. Boot Services 阶段的规则（写 `EfiMain` 必须遵守）

1. **`EfiMain` / MikanLoader 不能裸读写** 固件保留区、MMIO 硬件区 —— 只能走固件 API。  
2. 查内存布局、分配 Boot 期内存、读文件 → 调 **`gBS`** 下接口，如 **`GetMemoryMap()`**。  
3. 内存与硬件由 **固件统一管**；EFI 应用只在 **Loader 自身区（LoaderCode/Data）+ 开放 API** 内活动。

**例子（Ch1 就用了）：**  
`SystemTable->ConOut->OutputString(L"...")` = 走固件 **ConOut 协议** 打印 —— **不是** 自己往显存 MMIO 地址乱写。这就是 Boot Services 下的正确姿势。

→ Boot / Runtime 概念：[§2.4 Boot vs Runtime 服务](../section-2-4-Boot与Runtime服务.md)

---

### 4. 启动主线：Ch1 → Ch2 → 内核（别混阶段）

| 阶段 | 谁跑 | 做什么 | **不做什么** |
|------|------|--------|--------------|
| **Ch1** | 裸 C **`BOOTX64.EFI`** | `EfiMain` + **`ConOut` 打印** · 理解 PE/工具链 | ❌ **没有** GetMemoryMap · ❌ **没有** 加载 kernel · ❌ **没有** ExitBootServices |
| **Ch2** | **MikanLoader**（EDK II） | **`GetMemoryMap()`** · 导出 **memmap CSV** · （后续章）加载 **kernel.elf** | 本章先 **摸底内存** |
| **Ch2+ / Ch3+** | MikanLoader 继续演进 | 加载内核到 **Conventional** · **`ExitBootServices()`** · 跳内核 | Exit **之后** Boot 服务 API **不可用** |

**交接顺序（MikanLoader 完整路径）：**

```
GetMemoryMap()              → 拿到「地址清单」（Boot 期）
加载 kernel.elf 到 Conventional → 在可用 RAM 摆内核
ExitBootServices()          → 关闭 Boot Services；须再读 map 看哪些页已可回收
跳转入内核                   → 脱离 UEFI Boot 管控，OS 自管 Conventional 等
```

**Ch1 与 Ch2 分工：** Ch1 只证明 **「固件能加载你的 .efi 并打印」**；**读 map、加载内核、Exit** 是 **Ch2 MikanLoader** 的事 —— 不要写成「Ch1 就 GetMemoryMap 加载内核」。

→ Ch1 七步：[§5.1 UEFI 七步启动流程](../../chapter-01-hello-world/notes/section-5-1-UEFI七步启动流程.md) · [§5 索引](../../chapter-01-hello-world/notes/section-5-UEFI启动流程.md)

---

### 5. 总图（与 3.2 四层对照）

```
[ ① UEFI 固件区 ]     ← 管家办公区（Runtime + Boot Services + ACPI…）
[ ② MMIO / 保留区 ]   ← 设备、显存 — 不是堆内存
[ ③ BOOTX64.EFI ]     ← 客厅临时工作台（Hello / MikanLoader）
[ ④ 可用 RAM ]        ← 内核「办公室」主要从这里挑
        ↑
   GetMemoryMap / memmap CSV  — 每一段在表里都有类型与属性
```

---

← [3.2 RAM 四层占用](./section-3-2-RAM四层占用.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一篇 [3.4 地址清单与 UEFI 类型](./section-3-4-地址清单与UEFI内存类型.md)
