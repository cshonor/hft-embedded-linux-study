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

### 4. 从 Ch1 启动到 Ch2 交接（一条线）

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

→ Ch1 启动七步：[§5 UEFI 启动流程](../../chapter-01-hello-world/notes/section-5-UEFI启动流程.md)

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
