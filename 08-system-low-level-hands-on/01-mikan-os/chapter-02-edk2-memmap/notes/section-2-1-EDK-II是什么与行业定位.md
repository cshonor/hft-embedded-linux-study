## 2.1 EDK II 是什么 · 行业定位

> **§2 子笔记 1/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

### 基础定义

**EDK II**（**EFI Development Kit II**）— Intel 发起、由 **[TianoCore 开源社区](https://github.com/tianocore/edk2)** 维护的 **开源 UEFI 固件完整开发工具包**，也是全球 PC、服务器、嵌入式平台 **UEFI/PI 规范的标准参考实现**。

| 项 | 说明 |
|----|------|
| **开源仓库** | [github.com/tianocore/edk2](https://github.com/tianocore/edk2) |
| **协议** | BSD 开源 |
| **现实地位** | 现代主板 BIOS、虚拟机 UEFI 固件（**OVMF**）、大量自定义引导器 **基于 EDK II** 或其衍生栈 |

**简易类比：**

| | 角色 |
|---|------|
| **UEFI 规范** | 接口标准文档（类似 **HTTP 协议**） |
| **主板 UEFI 固件** | 真正跑在你机器上的 **实现** — `GetMemoryMap` 等 API **由它提供** |
| **EDK II** | 开源 **参考实现 + 全栈开发框架**（不只头文件） |

**一句话：** EDK II **不只是单纯的 C 库** —— 是 **工具链 + 标准库 + 驱动模块 + 固件构建系统** 的完整 UEFI 开发套件。

→ Loader 与 API 归属：[2.3](./section-2-3-MikanLoader是什么.md) · 两路线对照：[Ch1 §7](../../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

### 分层拆解：EDK II 里到底有什么

#### 1. 其中一块 ≈ 「标准 UEFI C 库」（写 `.efi` 程序会用到）

核心包 **`MdePkg`** = 全套头文件 + 基础库：

| 提供什么 | 例子 |
|----------|------|
| **结构体 / 类型** | `EFI_SYSTEM_TABLE`、`EFI_MEMORY_DESCRIPTOR`、`BootServices`、`GetMemoryMap` |
| **工具函数封装** | 字符串、Boot 期内存分配、`Print`、文件读写 |

**单独拎出 `MdePkg`，可以当成 UEFI 专用 C 库** —— 和 Ch1 **手写 `uefi.h` 片段**（见 [hello.c](../../chapter-01-hello-world/code/01-clang-minimal/hello.c) 顶部）干同一件事，只是 EDK II **官方完整实现**；Ch1 是 **从 MdePkg 同源规范里手动剥离极简子集**，用通用 clang/lld 编译，不拉 EDK II 构建树。

→ Ch1 详述：[§7 手写 uefi.h 与 MdePkg](../../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md#手写-uefih-与-edk-ii-mdepkg-uefih)

#### 2. 但整体远不止「库」— 三大核心组成

| # | 组成 | 干什么 |
|---|------|--------|
| **① 海量预制驱动 / Pkg 包** | USB、网卡、FAT、ACPI、TPM、平台初始化… | 主板厂 **拼出整板 BIOS** |
| **② BaseTools 专属构建链** | Python 脚本、AutoGen、GenFw、`.inf/.dsc/.dec` | **不能** 只用普通 `gcc`+Makefile 替代整套流程 |
| **③ 完整固件生成** | GenFds → FV → **Flash 镜像** | 烧进主板 ROM 的 **整板 UEFI**，不只单个 `.efi` |

→ 构建详表见下节「EDK II 完整提供什么」· [附录 C](../../appendix-C-edk2-files/)

#### 3. 核心疑问：能不能说 EDK II 是「库」？

| 视角 | 答案 |
|------|------|
| **局部** | **`MdePkg` ≈ UEFI 标准 C 库** |
| **整体** | **库 + 工具链 + 驱动模块 + 固件构建系统** — 不能简称为「一个库」 |

---

### EDK II 路线 vs Ch1 极简 Loader 路线

| | **EDK II 路线（工业 / 量产）** | **Ch1 极简路线（本仓库默认入门）** |
|---|-------------------------------|-------------------------------------|
| **依赖** | EDK II 全框架 · MdePkg · BaseTools · `.inf/.dsc` | **手写极简类型** · **clang + lld-link + Makefile** |
| **适合** | 主板固件、嵌入式整机 UEFI、OVMF、量产 Loader | **吃透原理** — PE、EfiMain、BootServices 调用 |
| **驱动** | 内置成熟 FAT/USB/ACPI 等 **Pkg 模块** | **只调主板已有固件的 API** — 不自带 EDK 驱动 |
| **本书** | **Ch2 MikanLoader** 按原书走 EDK II **工程化** | **Ch1 [01-clang-minimal](../../chapter-01-hello-world/code/01-clang-minimal/)** 默认先做 |

**关键澄清（易混）：**

- 你现在调的 **`GetMemoryMap`、`SystemTable`、`ConOut`** → **主板原生 UEFI 固件** 提供的标准 API，**不是 EDK II「运行时」给你的**。
- EDK II 的 **`MdePkg` / `<Uefi.h>`** → 把这套规范 **翻译成 C 头文件与库函数**，方便你写 Loader。
- **Ch2 用 EDK II** = 用 **工业级工程方式** 写 MikanLoader；**不是** 因为只有 EDK II 才能调 `GetMemoryMap` —— Ch1 手写声明 **理论上也能调同一套固件 API**。

**学习路线建议：**

1. **现阶段吃透原理** — Ch1 极简 **`uefi.h` + WSL clang** 足够理解启动链与固件 API。
2. **本书 Ch2** — 用 EDK II **重写 / 工程化** MikanLoader（`.inf/.dsc`、导出 memmap）— 对接原书与量产习惯。
3. **以后做商用嵌入式整机、定制服务器 BIOS** — 才必须深入 **EDK II 全套框架 + Pkg 拼固件**。

**与 Ch1 关系：** Ch1 用 **裸 C + 手写最小类型** 编出 `BOOTX64.EFI`；Ch2 纳入 **EDK II 工程体系** — 头文件、库、`.inf/.dsc` 构建 **一条链搞定**。

→ [Ch1 §7 两阶段全链路](../../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

### 核心用途（三类）

**（1）底层固件开发 — BIOS / UEFI 主体**

- 主板、服务器、工控机、笔记本 **原厂固件**
- AMI、Insyde、Phoenix 等商用 BIOS 厂商多在 EDK II 上 **二次定制** 硬件初始化
- 覆盖上电全流程：**SEC → PEI → DXE → BDS → RT**（平台初始化链路；MikanOS Loader 运行在 **DXE/BDS 之后的 UEFI 应用阶段**）

**（2）自定义 UEFI 应用与引导程序** ← **MikanOS 主线**

| 例子 | 说明 |
|------|------|
| **MikanLoader** | 本书引导加载器雏形 → [2.3](./section-2-3-MikanLoader是什么.md) |
| 系统安装器、硬件诊断、BIOS 设置 UI | 常见 UEFI 应用 |
| **QEMU OVMF** | 虚拟 UEFI 固件 — Ch1 `make run` / OVMF 即此类 |

**（3）驱动与安全组件**

PCIe / USB / 网卡 / 存储 **UEFI 驱动**；**Secure Boot**、TPM2、可信测量、固件签名校验等。

---

### EDK II 完整提供什么

| # | 内容 | 说明 |
|---|------|------|
| 1 | **标准头文件与协议** | UEFI Boot/Runtime Services、Protocol、PPI、GUID、PCD — 主要在 **MdePkg** |
| 2 | **分层标准库** | 内存、字符串、文件、网络、密码学、ACPI、图形输出等 **可复用固件库** |
| 3 | **构建系统 BaseTools / Stuart** | Python 工具链；**WSL / Linux / macOS** 可编；产出 **Flash 固件镜像** 或 **`.efi` 应用** |
| 4 | **模块化工程规范** | **Package / Module** + **`.inf` / `.dec` / `.dsc`** 管理依赖 — 见 [附录 C](../../appendix-C-edk2-files/) |
| 5 | **多架构** | IA32、**X64**、AArch64、RISC-V64、LoongArch64 等 |

---

### 配套文档入口（本仓库）

| 文档 | 内容 |
|------|------|
| [SETUP.md](../../SETUP.md) | **WSL2 + Ubuntu** 编译环境；Ch1 Clang/lld-link；Ch2+ EDK II 手动搭链 |
| [appendix-C EDK II 文件](../../appendix-C-edk2-files/) | **`.inf` / `.dsc` / `.dec`**、Package 目录结构 |

---

### 行业定位 · EDK I vs EDK II

| | **EDK II** |
|---|------------|
| **地位** | 当前固件开发 **事实工业标准** |
| **也用在哪里** | Chromebook、服务器 BMC、coreboot UEFI Payload、Linux **EFI Stub** 生态等 |
| **vs EDK I** | EDK II **重构架构** — 模块化更强、原生 **UEFI 2.x**、安全子系统完善；**完全取代** 旧版 EDK |

**HFT 读法：** 生产服务器 **UEFI → boot loader → OS** 这条链的「固件侧参考实现」就是 EDK II 体系；MikanOS 学的是 **同一套协议与构建模型** 的缩小版。

---

← [§2 索引](./section-2-EDK-II与MikanLoader.md) · [1. 本章定位](./section-1-本章定位.md) · 下一篇 [2.2 用 EDK II 重写 Hello World](./section-2-2-用EDK-II重写HelloWorld.md)
