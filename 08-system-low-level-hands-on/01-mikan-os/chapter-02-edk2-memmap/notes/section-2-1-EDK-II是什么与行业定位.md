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
| **EDK II** | 实现该标准的 **代码库 + 构建工具链**（类似 **Nginx / Apache** 之于 HTTP） |

**与 Ch1 关系：** Ch1 用 **裸 C + 手写最小类型** 编出 `BOOTX64.EFI`；Ch2 起纳入 **EDK II 工程体系** — 头文件、库、`.inf/.dsc` 构建 **一条链搞定**。

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
