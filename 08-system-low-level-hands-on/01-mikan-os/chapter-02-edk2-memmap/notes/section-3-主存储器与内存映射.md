# Ch 2 §3 主存储器与内存映射

> **MikanOS** · 原书第 2 章 · **🔴**

## 3. 主存储器与内存映射

---

### 先澄清：这里的「内存映射」指什么？

| ❌ **不是** | ✅ **是** |
|------------|----------|
| Linux 里 **动态库共享** 的 mmap | UEFI 固件给你画的一张 **「整台机器物理 RAM 使用地图」** |
| 进程虚拟地址空间 | 从 **物理地址 0** 起，整段地址空间 **按用途切成一段段** |

**一句话：** 固件告诉你 —— **哪块 RAM 已被占用、哪块还能用、每块该怎么用**（能执行？能写？是普通内存还是设备寄存器？）。

你后面 **`gBS->GetMemoryMap()`** 拿到的就是这张表的 **机器可读版本**；MikanLoader 再导出成 **memmap CSV** 方便人眼看。

---

### 一、主存储器（RAM）的软件视图

从软件角度，**主存储器 = 连续排列的字节**：

```
物理地址:  0x00000000  0x00000001  0x00000002  …  0xFFFFFFFF…
           ┌────┬────┬────┬─────
           │ B0 │ B1 │ B2 │ …     每个地址 1 字节
           └────┴────┴────┴─────
```

| 概念 | 说明 |
|------|------|
| **物理地址** | CPU / 内存控制器访问 RAM 的 **编号** |
| **字节寻址** | 最小可寻址单位 = **1 字节**（8 bit） |
| **容量** | 由硬件安装条数决定 — OS 需 **探测** 实际布局 |

→ [CSAPP Ch6 存储器层次](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) · [Hennessy Ch2 内存](../../../03-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/)

---

### 二、RAM 占用四层 —— UEFI 开机后的典型分层

RAM **并非一整块空闲** 给未来的 MikanOS。UEFI 启动到 **MikanLoader 运行时**，物理地址从低到高 **大致** 可理解成四层：

```
物理地址 ↑
         │
  ┌──────┴──────────────────────────────────────┐
  │ ④ 可用常规 RAM（EfiConventionalMemory）      │  ← 内核、栈、堆 **主要从这里挑**
  ├─────────────────────────────────────────────┤
  │ ③ 已加载的 EFI 应用（LoaderCode / LoaderData）│  ← bootX64.efi / MikanLoader **自己**
  ├─────────────────────────────────────────────┤
  │ ② 硬件保留区（MMIO、显存、设备映射…）         │  ← **不是** 普通内存，乱写会炸
  ├─────────────────────────────────────────────┤
  │ ① 低地址：UEFI 固件区（代码、ACPI 表…）       │  ← 固件核心数据
  └─────────────────────────────────────────────┘
        0x00000000
```

| 层 | 谁占的 | 典型内容 | 内核能当堆用吗？ |
|----|--------|----------|------------------|
| **① 固件区** | **UEFI 固件** | 固件代码、**ACPI 表**、启动早期数据 | ❌ **禁止** — 踩了就死机 |
| **② 硬件保留** | **主板/设备** | **显卡显存 (Framebuffer)**、**MMIO 设备寄存器** | ❌ **不是 RAM** — 写进去是 **控制硬件**，不是存变量 |
| **③ EFI 应用** | **你刚跑的 Loader** | `bootX64.efi` / **MikanLoader** 的代码段、数据段 | ⚠️ **暂时在用** — ExitBootServices 前别乱覆盖 |
| **④ 可用 RAM** | **尚未分配** | 后续 MikanOS 内核、页表、栈、堆 | ✅ **主要分配目标** |

**新手记住：** 写内核时 **不能随便 `malloc` 一块地址就写** —— 必须 **照着内存映射表**，只从标成 **「可用」** 的段里划。

---

### 三、固件 vs 已加载 EFI 应用 —— 内存里谁在哪？

> **直觉版（先记住）：** UEFI 固件是 **「底层管家」**；你编译的 **`BOOTX64.EFI`**（Ch1 Hello / Ch2 **MikanLoader**）是管家 **「请来的临时帮手」** —— 二者在内存里是 **两块完全隔离的区域**，不是混在一起的一坨代码。

#### 1. 比喻对照

| 比喻 | 对应什么 | 说明 |
|------|----------|------|
| **管家的办公区** | **UEFI 固件** 占用的内存 | 固件核心服务、硬件初始化代码、**ACPI 表**、**Memory Map** 等 **系统级数据**；权限最高，**你的 Loader 不能直接乱写** |
| **客厅临时工作台** | **已加载的 EFI 应用**（`.efi`） | 固件把 `BOOTX64.EFI` **整份 PE 镜像** 载入 RAM —— 代码/数据在 **单独一段**（上表 **③ LoaderCode / LoaderData**） |
| **你自己的办公室** | **MikanOS 内核**（Ch3+） | Loader 把 **kernel.elf** 加载进 **④ 可用 RAM**，长模式页表建好后正式运行 |
| **拆掉工作台、收回客厅** | **`ExitBootServices` 之后** | Boot 阶段占用的 **Loader / Boot Services 内存** 可在 map 里标成可用，内核 **回收复用** |

| 阶段 | 「帮手」在干什么 |
|------|----------------|
| **Ch1 Hello World** | 只在 **工作台** 上 `ConOut` 打印 —— 尚未读 map、未加载内核、未 **ExitBootServices** |
| **Ch2 MikanLoader** | **`GetMemoryMap`** → 导出 CSV → 加载 **kernel.elf** → **ExitBootServices** → 完整「临时工 → 内核」交接 |

#### 2. 技术版：不是「低地址 = 固件、后面 = 应用」

物理地址 **不是** 简单从 0 起「固件占前半、应用占后半」。固件用 **Memory Map** 按 **类型** 标记 **每一段** RAM（见 [§5 UEFI 内存类型](#五uefi-内存类型与属性对照-csv)）：

| 类型（常见） | 对应四层 | 谁占 | 内核能随便写吗？ |
|--------------|----------|------|------------------|
| **EfiRuntimeServicesCode/Data** | ① 固件 | OS 运行后 **仍保留** 的固件部分 | ❌ **长期勿动** |
| **EfiBootServicesCode/Data** | ① 固件 | 仅 **Boot 阶段** 的固件服务 | ❌ Boot 期间固件管；**ExitBootServices 后** 常可回收 |
| **EfiLoaderCode / LoaderData** | ③ EFI 应用 | **已加载的 `.efi`** | ⚠️ **正在跑** 时别覆盖；交接后可回收 |
| **EfiConventionalMemory** | ④ 可用 RAM | 空闲 | ✅ 内核、栈、堆 **主要从这里划** |
| **MMIO / Reserved** | ② 硬件保留 | 设备寄存器、显存 | ❌ **不是普通 RAM** |

#### 3. Boot Services 期间的「权限直觉」

- 你的 **`EfiMain` / MikanLoader`** **不能** 直接读写 **管家办公区** —— 只能经 **`SystemTable` → `gBS` / 协议**（如 `ConOut`、`GetMemoryMap`）**间接** 调固件。
- 固件 **定布局、管硬件**；EFI 应用 **只在 Loader 区 + 固件允许的 API** 里活动。
- 这就是为什么 Ch1 手写 **`SystemTable->ConOut->OutputString`** 就够了 —— 你 **借管家的接口** 干活，而不是自己 mmap 固件区。

#### 4. 从 Ch1 启动到 Ch2 交接（一条线）

```
[Ch1 §5 七步]
  ⑤ 固件加载 BOOTX64.EFI 到 RAM     → 划出「客厅工作台」（LoaderCode/Data）
  ⑥ 跳 EfiMain                      → 临时帮手开始跑

[Ch2 本章]
  GetMemoryMap()                     → 拿到整张「地址清单」
  加载 kernel.elf 到 Conventional    → 在「可用 RAM」里摆内核
  ExitBootServices()                 → 关掉 Boot Services；Loader 区等多可收回
  跳内核                             → 「办公室」正式启用
```

#### 5. 总图（与 §二 四层对照）

```
[ ① UEFI 固件区 ]     ← 管家办公区（Runtime + Boot Services + ACPI…）
[ ② MMIO / 保留区 ]   ← 设备、显存 — 不是堆内存
[ ③ BOOTX64.EFI ]     ← 客厅临时工作台（Hello / MikanLoader）
[ ④ 可用 RAM ]        ← 内核「办公室」主要从这里挑
        ↑
   GetMemoryMap / memmap CSV  — 每一段在表里都有类型与属性
```

→ Ch1 启动七步背景：[§5 UEFI 启动流程](../chapter-01-hello-world/notes/section-5-UEFI启动流程.md)

---

### 四、内存映射的核心作用 —— 「地址清单」

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

---

### 五、UEFI 内存类型与属性（对照 CSV）

`GetMemoryMap()` 返回 **`EFI_MEMORY_DESCRIPTOR`** 数组 —— 每一段连续物理区域一行。

| 类型（常见） | 对应上面哪层 | 含义 |
|--------------|--------------|------|
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

### 六、和动态库 mmap / 虚拟内存的区别（防混）

| 阶段 | 你在学什么 |
|------|------------|
| **Ch 2（本章）** | **物理 RAM 布局真相** — 固件交底 |
| **Ch 8+** | 在 **可用物理区** 里做 **线性 / 物理页分配** |
| **Ch 19** | **分页** — 虚拟地址 ↔ 物理页（那时才有「进程地址空间」） |
| **Linux mmap** | 用户态 **虚拟内存** 映射 — **远在** 物理摸底之后 |

```
Ch 2  memmap CSV     — 「物理世界真相」快照
  ↓
Ch 8  物理内存管理   — 只在 Conventional 区内分配
  ↓
Ch 19 分页           — 每个进程看到的虚拟地址
```

→ 对照 Linux **`/proc/iomem`**、x86 **e820 表**（Legacy BIOS 传递的同类信息）

---

### 七、自检（概念）

- [ ] 能说出 **四层 RAM 占用** 各是谁、内核能不能用  
- [ ] 能用 **管家 / 工作台 / 办公室** 比喻说清 **固件 vs `.efi` vs 内核** 在内存里的关系  
- [ ] 能解释 **GetMemoryMap ≠ 动态库共享**  
- [ ] 知道 **LoaderCode：可执行、别当普通可写 RAM**  
- [ ] 知道 **`ExitBootServices` 后** 哪些 Boot 期内存 **可回收**  
- [ ] 知道内核 **只能从 Conventional 等允许类型** 里划内存  

---

← [2. EDK II](./section-2-EDK-II与MikanLoader.md) · 下一节 [4. GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md)
