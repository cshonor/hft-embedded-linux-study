## 4. GetMemoryMap 与导出 memmap

> **§4 子笔记** · [§3 内存类型](./section-3-4-地址清单与UEFI内存类型.md) · [Boot vs Runtime](../section-2-4-Boot与Runtime服务.md)

---

### 通俗版 · 先建立直觉

#### 1. 本质定位

**`GetMemoryMap` 是 UEFI Boot Services 里的一个固件 API** —— 让你的 **`bootx64.efi` / MikanLoader** 向主板 UEFI 固件 **索要整张整机物理内存分布图**。

不是内核功能，不是 Linux `mmap` —— **只有 Boot 阶段、通过 `gBS` 调用**（→ [3.5 与 mmap 区别](./section-3-5-与mmap区别与自检.md)）。

#### 2. 返回什么？

调用后得到 **`EFI_MEMORY_DESCRIPTOR` 数组** —— 就是你 CSV / [3.4 类型表](./section-3-4-地址清单与UEFI内存类型.md) 里每一行的来源：

| 每条记录 | 含义 |
|----------|------|
| **起始物理地址** | 这段从哪开始 |
| **长度** | 这段多大 |
| **Type** | Conventional / LoaderCode / MMIO / Runtime … |
| **Attribute** | 可执行、可写、缓存策略 … |

**白话：** 固件把 **全部 RAM + MMIO 硬件地址** 分块贴好标签，做成 **清单** 交给你的引导程序。

#### 3. 为什么不能自己猜地址？

| ❌ 硬编码 | ✅ 查表 |
|-----------|---------|
| 「0x100000 起一定是空闲内存」 | 换主板/显卡/内存条 **布局全变** |
| 可能盖住 Runtime / ACPI | 只从 **Conventional** 划给内核 |
| 可能写到 MMIO 当 RAM | MMIO / Reserved **碰了黑屏** |

**开发铁则：** 分配池 **默认只认 `EfiConventionalMemory`** —— 其余 Type 特殊处理（→ [3.4](./section-3-4-地址清单与UEFI内存类型.md)）。

#### 4. MikanLoader 里的使用时机（完整链）

```
EfiMain
  → SystemTable->BootServices (gBS)
  → GetMemoryMap()           ← 本章：导出 memmap CSV
  → 遍历表，统计 Conventional 段
  → （Ch3+）加载 kernel.elf 到 Conventional
  → ExitBootServices()       ← 须带 MapKey；Boot API 此后不可用
  → 再读一次 map（或沿用 Exit 前保存的表）← 看 Boot/Loader 区是否变可回收
  → 跳内核，Ch8+ 在 Conventional 里做分配器
```

**Ch2 本章做到：** 第 2 步 + 导出 CSV —— **先建立「物理世界账本」**，不在此章 Exit 或加载内核。

#### 5. HFT / 嵌入式延伸

| 场景 | 同样逻辑 |
|------|----------|
| **HFT 裸金属 / 定制内核** | 划 **大页**、**隔离网卡 MMIO** — 避免把 BAR 当堆；低延迟路径不能踩错区 |
| **ARM64 UEFI（如无人机）** | 同样有 **GetMemoryMap** — 划分飞控 MMIO、预留区，**Conventional** 给 Linux/自研内核 |

**一句话：** `GetMemoryMap` = UEFI 给引导程序的 **「内存查询接口」** —— 拿到 **完整分区清单**，分清 **能用 / 固件保留 / 硬件保护区**，避免乱踩地址崩溃。

---

### 一、调用链概览（工程版）

```
MikanLoader (EfiMain)
    ↓
gBS  ←  UEFI Boot Services 全局表
    ↓
GetMemoryMap()  — 获取内存描述符数组
    ↓
Simple File System Protocol  — 访问 U 盘
    ↓
EFI_FILE_PROTOCOL  — 创建/写入 memmap 文件
    ↓
CSV 文本写入 U 盘根目录（或指定路径）
```

---

### 二、`gBS->GetMemoryMap()`

| 要素 | 说明 |
|------|------|
| **`gBS`** | **全局 Boot Services 表** 指针 — EDK II 中常用全局变量 |
| **GetMemoryMap** | 填充 **EFI_MEMORY_DESCRIPTOR** 数组 |
| **典型参数** | MapSize、MapKey、DescriptorSize、DescriptorVersion |
| **MapKey** | 映射版本键 — 内存分配后可能变化，后续 **ExitBootServices** 前需一致 |

**输出：** 若干条描述符，每条 = **一段连续物理地址 + 类型 + 属性** —— 即 [3.2 四层 RAM 地图](./section-3-2-RAM四层占用.md) 的 **机器可读清单**。

| CSV 列（概念） | 对应 |
|----------------|------|
| 起始地址、长度 | 这一段 **物理 RAM** 占哪 |
| **Type** | `Conventional` / `LoaderCode` / `ACPI` … |
| **Attribute** | 可执行、可写、缓存策略 … |

**读 CSV 时：** 找 **`LoaderCode`** 行 —— 那是 **MikanLoader 自己的代码段**，属性常含 **可执行、不可写**；找 **`ConventionalMemory`** —— 那是你 **Ch 8 内核该从这里划页** 的候选区。

```c
// 示意 — 实际需按 UEFI 规范处理缓冲区大小与重试
Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey,
                           &DescriptorSize, &DescriptorVersion);
```

**注意：** 首次调用常因缓冲区不足返回 **`EFI_BUFFER_TOO_SMALL`** — 需先查询 `MapSize` 再分配缓冲（书中完整流程）。

---

### 三、写入 U 盘：文件协议

| 协议 | 作用 |
|------|------|
| **Simple File System Protocol** | 挂载 FAT 等卷 |
| **`EFI_FILE_PROTOCOL`** | 打开/创建/读/写 **文件** |

**本章操作：**

1. 定位启动用 U 盘（或 QEMU 虚拟 FAT 盘）
2. 创建文件 **`memmap`**
3. 将每条内存描述符格式化为 **CSV 行** 写入

**CSV 价值：**

| 用途 | 说明 |
|------|------|
| **人工分析** | Excel / 编辑器查看空洞与保留区 |
| **后续开发** | 内核初始化时 **读入** 相同布局（或运行时再次 GetMemoryMap） |
| **调试** | 真机 vs QEMU 内存差异对比 |

---

### 四、ExitBootServices 预告（概念）

OS 正式接管前，Loader 需调用 **`ExitBootServices(MapKey)`** — 此后 **Boot Services（含 GetMemoryMap）不可用**。

| 阶段 | 内存信息 |
|------|----------|
| **Loader 期（Ch2 本章）** | 随时 `GetMemoryMap` · 导出 CSV |
| **Exit 前** | 保存 map，或 **Exit 后再读一次**（若仍可用 Boot API 的窗口内） |
| **内核期（Ch8+）** | 用 **Exit 前/后保存的表** 初始化物理页池 — 不能再调 `gBS` |

→ 类型与回收规则：[3.4 生命周期总表](./section-3-4-地址清单与UEFI内存类型.md) · Ch3+ 引导流程

---

### 五、实验自检

- [ ] 能一句话说清 **GetMemoryMap = Boot 期向固件要内存清单**
- [ ] 能说出 **每行含地址、长度、Type、Attribute**
- [ ] 能解释 **为什么不能硬编码 0x100000**
- [ ] QEMU 运行 MikanLoader 后 U 盘映像中出现 **`memmap`**
- [ ] CSV 中含 **ConventionalMemory** 行 — 地址范围合理
- [ ] 存在 **Reserved / ACPI / MMIO** 等类型 — 非全盘可用

---

← [3.5 与 mmap 区别 · 自检](./section-3-5-与mmap区别与自检.md) · [§3 索引](./section-3-主存储器与内存映射.md) · 下一节 [5. 指针基础](./section-5-C指针基础.md)
