## 2.3 MikanLoader 是什么 · 它到底加载什么

> **§2 子笔记 3/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

### 先纠正两个误区

#### ① UEFI 不是 C 语言库

| ❌ 误区 | ✅ 正解 |
|---------|---------|
| UEFI = 某个 `#include <Uefi.h>` 的第三方库 | UEFI = **硬件固件标准规范**（开机流程、内存类型、文件系统、`.efi` 格式…） |
| 写 EFI 程序 = 链接一个 `.a` | 规范定义 **固件提供的 C 调用接口**：`SystemTable`、`BootServices`、`RuntimeServices` |
| Ch1 手写类型 = 随便编的 | Ch1 **手写 `uefi.h` 片段** = 把规范里的 **结构体 / 函数声明抄出来**，方便 C 调固件 |

**EDK II** 则是实现这套规范的 **工具链 + 头文件 + 库**（→ [2.1](./section-2-1-EDK-II是什么与行业定位.md)），不是 UEFI 本身。

#### ② 自定义 EFI 应用 = 操作系统引导加载器（Loader）

你编译出的 **`BOOTX64.EFI` / MikanLoader** = **自定义 UEFI 应用**，行业统称 **Boot Loader（引导加载器）**。

| | Loader | 内核 |
|---|--------|------|
| **是什么** | **中转工具** — 依托固件 API 干活 | **OS 本体** — 接管硬件与内存 |
| **跑在哪** | Boot Services 期 · `EfiLoaderCode` 内存 | Exit 后 · 自管 Conventional 等 |
| **Ch1 Hello** | 只打印 — **Loader 雏形** | 还没有 |
| **Ch2+** | GetMemoryMap、读盘、加载 kernel | `kernel.elf`（Ch3+） |

---

### Loader 完整工作链路：加载两类东西

#### 第一层（核心目标）：从磁盘加载 **操作系统内核镜像**

1. UEFI 固件自带 **FAT 文件驱动** — Loader 调 **固件文件 API**（`Simple File System` / `EFI_FILE_PROTOCOL`），读磁盘上的 **`kernel.elf`**（路径不在 `/EFI/BOOT/` 固定名里，由你工程约定）；
2. 把内核读到 **筛选出的空闲物理内存**（**`EfiConventionalMemory`**）；
3. 记录 **内核入口地址**、**占用内存范围**，为 **跳转执行** 做准备。

→ Ch3 详读 ELF 加载：[Ch3 第一个内核](../../chapter-03-bootloader-display/notes/section-3-第一个内核与ELF加载.md)

#### 第二层（铺垫）：加载 / 获取 **整机硬件资源描述**

| 数据 | 怎么拿 | 交给内核干什么 |
|------|--------|----------------|
| **内存分区表** | `GetMemoryMap()` → 筛 **Conventional** | Ch8 物理页池 · 知道哪不能踩 |
| **FrameBuffer** | **GOP** 协议 | 显存地址、分辨率、像素格式 → 屏幕输出 |
| **ACPI 等** | 固件已映射 / 表地址 | 电源、中断控制器等（Ch11+ 定时器/ACPI） |

**Ch2 本章做到：** 第二层里的 **GetMemoryMap + 导出 CSV** —— 第一层（加载 kernel）在 **Ch3+**。

→ [§4 GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md) · [§3.4 内存类型](./section-3-4-地址清单与UEFI内存类型.md)

---

### 完整生命周期

```
① 上电 → 主板 UEFI 固件运行
      ↓
② 固件在 FAT ESP 找到 BOOTX64.EFI（你的 Loader）
      → 载入 EfiLoaderCode 内存 → 跳 EfiMain
      ↓
③ Loader 借 BootServices API：
      · 读盘 → 把 kernel 载入 Conventional
      · GetMemoryMap / GOP / … → 收集硬件信息
      · ExitBootServices() → 释放 Boot/Loader 临时区（须 MapKey；Exit 后重读 map）
      ↓
④ 跳转内核入口 → 脱离 UEFI 管控，内核自管硬件与内存
```

| 章 | Loader 停在哪一步 |
|----|-------------------|
| **Ch1** | ② 后只 **ConOut 打印** — 无 GetMemoryMap、无 kernel |
| **Ch2** | ③ 中 **GetMemoryMap + memmap CSV** |
| **Ch3+** | ③ 完整：**kernel + GOP + Exit + 跳内核** |

→ Boot/Runtime 分工：[2.4](./section-2-4-Boot与Runtime服务.md) · 内存隔离：[3.3](./section-3-3-固件与EFI应用内存隔离.md)

---

### 一句话（你最关心的问题）

**MikanOS 引导程序（自定义 EFI 应用 / MikanLoader）的核心任务：**

> 从磁盘 **加载 OS 内核到物理内存**，同时 **收集内存与硬件信息一并交给内核**。

**UEFI** = 主板底层固件，给 Loader 提供 **读盘、查内存、操作屏幕** 的 API —— **本身不是 C 库**。

```
MikanLoader.efi（UEFI · Boot Services 期）
    ↓ 加载 + 交底
MikanOS kernel.elf（Ch3+ 起 · Exit 后由内核接管）
    ↓
Ch8 内存管理 · Ch19 分页 · …
```

---

← [2.2 用 EDK II 重写 Hello World](./section-2-2-用EDK-II重写HelloWorld.md) · [§2 索引](./section-2-EDK-II与MikanLoader.md) · 下一篇 [2.4 Boot vs Runtime 服务](./section-2-4-Boot与Runtime服务.md)
