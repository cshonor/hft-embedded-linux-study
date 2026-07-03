# Ch 3 · 完整流程拆解：MikanLoader + GOP → kernel main

> **口述通读版** · 从上电到内核接管显存  
> **三问详解：** [§六 Q1 kernel.elf / Q2 GOP / Q3 为何要多 Loader](#六三个核心问题详解qa)  
> 细分：[§3.6 ELF 六步](./section-3-6-MikanLoader加载流程.md) · [§4 GOP 画布规格](./section-4-GOP与帧缓冲区.md) · [§5 KernelMain](./section-5-KernelMain与错误处理.md)

---

## 一、三个核心名词（MikanOS 定义）

| 名词 | 是什么 |
|------|--------|
| **MikanLoader（Miniload）** | 全书自定义 **UEFI 应用**（`.efi`）— 跑在 UEFI 固件里，是 **内核与固件之间的桥梁**；角色 ≈ 现代 Linux 的 **systemd-boot / GRUB**（更轻） |
| **`kernel.elf`** | 你手写的 **64 位 OS 内核** — **ELF64** 可执行文件（非裸 `.bin`），分段存放代码 / 数据 / BSS |
| **GOP** | **Graphics Output Protocol** — UEFI 标准图形协议，替代老式 VGA 文本；给出 **帧缓冲物理基址、分辨率、像素格式**（→ [§4 不传截图](./section-4-GOP与帧缓冲区.md#先纠正直觉gop-传的不是屏幕画面)） |

---

## 二、完整执行流程（上电 → kernel main）

```
① UEFI 固件上电、初始化硬件
        ↓
② 从 ESP 加载 MikanLoader.efi → UefiMain（EDK II；Ch1 裸 C 叫 EfiMain）
        ↓
③ Loader 读盘 + 解析 + 加载 kernel.elf（ELF 六步 — §3.6）
        ↓
④ Loader 调 GOP → 打包 FrameBufferConfig / BootInfo（地址+格式，非像素）
        ↓
⑤ ExitBootServices() — 释放 Boot 期资源，UEFI API 此后不可用
        ↓
⑥ 带参数跳 kernel 入口（KernelMain / main）— 内核自己写显存
```

### 步骤 1 · UEFI 固件启动，执行 MikanLoader

上电 → UEFI 初始化 **内存、显卡、磁盘** → 从 **ESP** 读 **`MikanLoader.efi`**（或 `BOOTX64.EFI`）→ 进入 Loader 入口。

| 工具链 | 入口符号 |
|--------|----------|
| EDK II（本书 Ch2+） | **`UefiMain`** |
| Ch1 裸 C | **`EfiMain`** |

### 步骤 2 · 读取并解析 `kernel.elf`

1. 调 **UEFI Boot Services**（`EFI_FILE_PROTOCOL` 等），从 ESP 读 **`kernel.elf` 完整二进制**；
2. 解析 **ELF64 头 + Program Header**：
   - 区分 **PT_LOAD** 段（可读 / 可写 / 可执行）；
   - 在 **`EfiConventionalMemory`** 分配物理页（→ [Ch2 §3.4](../../chapter-02-edk2-memmap/notes/section-3-4-地址清单与UEFI内存类型.md)）；
   - 把各段搬到链接脚本要求的地址；
   - 记录 **`e_entry`** — 后续跳转目标。

→ 详 [§3.6 六步加载](./section-3-6-MikanLoader加载流程.md)

### 步骤 3 · 调 GOP，获取帧缓冲 **硬件参数**

通过 **`LocateHandleBuffer` / `OpenProtocol`** 找到 **GOP**：

| 读出 | 用途 |
|------|------|
| **`FrameBufferBase`** | 显存 **物理起始地址** — 写这块内存 = 改像素 |
| **分辨率、Stride、PixelFormat** | 画布尺寸与 **BGR/RGB** 字节序 |
| **FrameBufferSize** | 显存总字节数 |

打包为 **`FrameBufferConfig`**（BootInfo 一部分），**留给内核** — **不是**把当前屏幕画面拷过去。

→ 详 [§4 GOP](./section-4-GOP与帧缓冲区.md)

### 步骤 4 · `ExitBootServices()` — 移交硬件控制权

- 释放 UEFI **Boot 期**占用的内存 / 驱动资源；
- 之后 **`gBS` 打印、读盘、GOP 等 API 全部失效**；
- 硬件控制权交给 **你的内核**（Runtime Services 仍有限可用，全书后续再讲）。

须带正确的 **`MapKey`**（→ [Ch2 GetMemoryMap](../../chapter-02-edk2-memmap/notes/section-4-GetMemoryMap与导出memmap.md)）。

### 步骤 5 · 跳转内核，传递 BootInfo / GOP 参数

1. Loader 把 **帧缓冲配置、内存分布（MemoryMap）、ACPI 表地址** 等作为 **函数参数**（或固定内存块指针）；
2. 跳转到 **`kernel.elf` 的 `e_entry`** → 通常进入 **`KernelMain`**；
3. 内核 **一启动就能按 `FrameBufferBase` 写显存**，不必再调 UEFI 显卡 API。

```cpp
// 示意 — 原书 KernelMain 签名随章节扩展
extern "C" void KernelMain(
    const FrameBufferConfig* fb,
    const MemoryMap* memmap,
    void* acpi_table,
    /* … */);
```

---

## 三、为何这么设计？（HFT / 嵌入式 Linux）

| 价值 | 说明 |
|------|------|
| **固件 / 内核边界清晰** | UEFI 只做 **硬件枚举 + 加载**；内核只做 **OS 逻辑**，不内嵌 UEFI 协议栈 |
| **与 Linux 启动对齐** | **GRUB / systemd-boot** 同样读 GOP / DTB，把 **帧缓冲 / 内存信息** 交给内核 — 服务器、ARM64 板卡标准路径 |
| **知识点可复用** | **ELF 加载** ↔ 动态链接 / `readelf`；**GOP** ↔ **`/dev/fb0`**；**ExitBootServices** ↔ 裸机服务器裁固件服务 |

---

## 四、与 02 川合 OS（BIOS 实模式）对比

| | **30 天 OS（Legacy BIOS）** | **MikanOS Ch3（UEFI 64 位）** |
|---|---------------------------|------------------------------|
| 固件 | 16 位实模式 BIOS | **UEFI 长模式** |
| 引导 | **512B IPL** 引导扇区 | **`.efi` Loader** + FAT 路径 |
| 内核格式 | 裸 bin / flat | **ELF64** |
| 显示 | 简陋 **VGA 文本** | **GOP 帧缓冲**（像素级） |
| 适用 | 启蒙、历史 | **x86_64 服务器 / ARM64 UEFI** — 更贴现代栈 |

→ [Ch1 §1.四 IPL vs BOOTX64](../../chapter-01-hello-world/notes/section-1-本章定位.md#ipl-与-bootx64efi功能等价形态不同)

---

## 五、一句话总结

**MikanLoader** 先从磁盘 **读出并解析 ELF 内核**，再通过 **GOP** 拿到 **显存地址与格式**（非屏幕快照），**ExitBootServices** 后带着 **BootInfo** 跳进 **kernel main** — 内核 **直接接管显存** 作画。

---

## 六、三个核心问题详解（Q&A）

### Q1 · `kernel.elf` 有什么作用？为什么需要它？

**`kernel.elf` = 你写的完整 OS 内核**，不是裸 `.bin`，而是带 **ELF64 标准结构** 的可执行文件：

| ELF 提供什么 | 作用 |
|--------------|------|
| **分段** | `.text` / `.rodata` / `.data` / `.bss` — 代码、只读常量、已初始化全局、未初始化区 |
| **`e_entry`** | ELF 头记录 **内核入口地址** — Loader 知道最后 **跳哪里** |
| **Program Header（PT_LOAD）** | 每段 **文件偏移 → 内存虚拟/物理地址**、权限（R/W/X） |
| **虚拟地址布局** | 内核常链接在 **高虚拟地址**；Loader 按 ELF **分配物理页、搬段、清 BSS**（Ch3 早期可能尚未开分页，仍按链接地址放置 — 见 §3.6） |

**UEFI 固件只能直接跑 `.efi` 应用**，**不能**解析、加载自定义 ELF 内核 —— 必须靠 **MikanLoader（.efi 中间层）** 读盘、解析、搬段、跳转。

| | **裸 bin** | **kernel.elf（ELF64）** |
|---|-----------|-------------------------|
| 结构 | 纯二进制流 | 头 + 段表 + 符号 |
| 入口 | 硬编码地址 | **`e_entry`** |
| 扩展性 | 极差 | **Linux `vmlinux`、ARM 嵌入式** 通用 |
| 谁加载 | 引导扇区手写 | **Loader 按 Program Header** |

→ 详 [§3.1 kernel.elf 定义](./section-3-1-kernel.elf基础定义与核心作用.md) · [§3.6 六步加载](./section-3-6-MikanLoader加载流程.md)

---

### Q2 · GOP 为什么能和 Miniload 通信，不能和内核直接通信？

#### ① 通信前提：UEFI Boot Services 仍在

上电 → 固件初始化显卡 → 封装成 **GOP 标准协议**。  
所有 **EFI 程序**（含 MikanLoader）通过 **`gBS->LocateHandleBuffer` / `OpenProtocol`** 调 GOP — 获取显存地址、分辨率等。  
此阶段 **硬件 + GOP 驱动 + API 都由 UEFI 接管**；Miniload 只是 **调用者**。

#### ② 内核不能直接调 GOP 的核心：`ExitBootServices()`

Miniload 加载完内核、收齐硬件信息后执行 **`ExitBootServices()`**：

| 一旦发生 | 后果 |
|----------|------|
| Boot Services 销毁 | **GOP、磁盘、Print、`GetMemoryMap` 等全部失效** |
| 资源回收 | 固件驱动退出；**硬件所有权交给内核** |
| 内核运行期 | **无法再调任何 UEFI 接口** — 只能使用 Loader **提前打包** 的 `FrameBufferBase` 等参数 **直接写显存** |

**物业类比：**

| 角色 | 对应 |
|------|------|
| **UEFI 固件** | 物业 — 提供 **GOP 控制面板** |
| **MikanLoader** | 中介 — 还在物业管辖时 **操作面板、抄下屏幕地址** |
| **`ExitBootServices`** | 物业下班关门 — 面板没了 |
| **内核** | 业主 — 只能拿中介 **提前抄的参数** 自己画画布 |

→ 详 [§4 GOP 不传截图](./section-4-GOP与帧缓冲区.md#先纠正直觉gop-传的不是屏幕画面) · [Ch2 §2.4 Boot vs Runtime](../../chapter-02-edk2-memmap/notes/section-2-4-Boot与Runtime服务.md)

---

### Q3 · UEFI 为何不能直接加载内核，非要 Miniload 这一层？

| # | 原因 |
|---|------|
| **1. 格式不兼容** | UEFI **原生只认 `.efi`**（PE32+ 等 EFI 规范程序），**不会**解析 ELF64 的段表、虚拟布局 — 无法把内核 **正确** 载入内存 |
| **2. 职责分离** | **固件**：硬件枚举、磁盘、标准协议（GOP/内存/文件）；**Loader**：解析 ELF、收集 BootInfo、准备跳转环境；**内核**：只管 OS 逻辑，**不内嵌 UEFI 协议栈** |
| **3. 灵活可换** | 换内核、改内存布局、增传参 — **只改 Miniload**，不动主板 **Flash 里的 UEFI** |

**Linux 同源：** **systemd-boot、GRUB、rEFInd** = UEFI 下的中间加载器，负责 **解析 Linux ELF 内核** — 与 Miniload **同一定位**。

→ [Ch2 §2.3 MikanLoader 是什么](../../chapter-02-edk2-memmap/notes/section-2-3-MikanLoader是什么.md) · [Ch1 §7 裸 C vs EDK II](../../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

### 三问串成一条链（再收束）

```
主板 UEFI 启动
  → 运行 .efi Miniload
  → Miniload 调 GOP 拿显存参数 + 解析磁盘 kernel.elf 分段载入
  → 打包 BootInfo
  → ExitBootServices（UEFI API 全灭）
  → 跳内核入口 — 内核靠提前拿到的 FrameBufferBase 独立操控屏幕
```

---

### 口述巩固

1. **MikanLoader 是什么？** — UEFI **`.efi` 加载器**，≈ GRUB，不是内核。  
2. **`kernel.elf` 干什么？** — **ELF64 内核容器**；固件 **不会** 解析，须 Loader 搬段 + 跳 **`e_entry`**。  
3. **GOP 传截图吗？** — **否**；传 **FrameBufferBase + 分辨率 + 格式**。  
4. **内核为何不能调 GOP？** — **`ExitBootServices` 后 Boot API 全失效** — 须 Loader 提前打包参数。  
5. **为何不能省 Loader？** — UEFI **只跑 `.efi`**，不解析 ELF 内核；职责分离 + 可换内核。  
6. **和 `/dev/fb0`？** — 同源 — **显存地址 + 写内存出图**（§4）。

---

← [§1 本章定位](./section-1-本章定位.md) · [Ch3 README](../README.md) · 下一节 [§2 QEMU](./section-2-QEMU监视器与寄存器.md)
