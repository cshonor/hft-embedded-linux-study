## 2. EDK II 与 MikanLoader

---

### 一、EDK II 是什么

#### 1. 基础定义

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

**与 Ch1 关系：** Ch1 用 **裸 C + `<Uefi.h>` 最小声明** 编出 `BOOTX64.EFI`；Ch2 起纳入 **EDK II 工程体系** — 头文件、库、`.inf/.dsc` 构建 **一条链搞定**。

---

#### 2. 核心用途（三类）

**（1）底层固件开发 — BIOS / UEFI 主体**

- 主板、服务器、工控机、笔记本 **原厂固件**
- AMI、Insyde、Phoenix 等商用 BIOS 厂商多在 EDK II 上 **二次定制** 硬件初始化
- 覆盖上电全流程：**SEC → PEI → DXE → BDS → RT**（平台初始化链路；MikanOS _LOADER 运行在 **DXE/BDS 之后的 UEFI 应用阶段**）

**（2）自定义 UEFI 应用与引导程序** ← **MikanOS 主线**

| 例子 | 说明 |
|------|------|
| **MikanLoader** | 本书引导加载器雏形 |
| 系统安装器、硬件诊断、BIOS 设置 UI | 常见 UEFI 应用 |
| **QEMU OVMF** | 虚拟 UEFI 固件 — Ch1 `make run` / OVMF 即此类 |

**（3）驱动与安全组件**

PCIe / USB / 网卡 / 存储 **UEFI 驱动**；**Secure Boot**、TPM2、可信测量、固件签名校验等。

---

#### 3. EDK II 完整提供什么

| # | 内容 | 说明 |
|---|------|------|
| 1 | **标准头文件与协议** | UEFI Boot/Runtime Services、Protocol、PPI、GUID、PCD — 主要在 **MdePkg** |
| 2 | **分层标准库** | 内存、字符串、文件、网络、密码学、ACPI、图形输出等 **可复用固件库** |
| 3 | **构建系统 BaseTools / Stuart** | Python 工具链；**Windows / Linux / macOS** 可编；产出 **Flash 固件镜像** 或 **`.efi` 应用** |
| 4 | **模块化工程规范** | **Package / Module** + **`.inf` / `.dec` / `.dsc`** 管理依赖 — 见 [附录 C](../../appendix-C-edk2-files/) |
| 5 | **多架构** | IA32、**X64**、AArch64、RISC-V64、LoongArch64 等 |

---

#### 4. 配套文档入口（本仓库）

| 文档 | 内容 |
|------|------|
| [SETUP.md](../../SETUP.md) | **WSL2 + Ubuntu** 编译环境；Ch1 Clang/ld.lld；Ch2+ EDK II 手动搭链 |
| [appendix-C EDK II 文件](../../appendix-C-edk2-files/) | **`.inf` / `.dsc` / `.dec`**、Package 目录结构 |

---

#### 5. 行业定位 · EDK I vs EDK II

| | **EDK II** |
|---|------------|
| **地位** | 当前固件开发 **事实工业标准** |
| **也用在哪里** | Chromebook、服务器 BMC、coreboot UEFI Payload、Linux **EFI Stub** 生态等 |
| **vs EDK I** | EDK II **重构架构** — 模块化更强、原生 **UEFI 2.x**、安全子系统完善；**完全取代** 旧版 EDK |

**HFT 读法：** 生产服务器 **UEFI → boot loader → OS** 这条链的「固件侧参考实现」就是 EDK II 体系；MikanOS 学的是 **同一套协议与构建模型** 的缩小版。

→ 两阶段全链路 [Ch1 §7](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) · 环境 [SETUP.md](../../SETUP.md) · 元文件 [appendix-C](../../appendix-C-edk2-files/)

---

### 二、用 EDK II 重写 Hello World

Ch 1 已用 C + Clang/**ld.lld** 写出 Hello World；本章改用 EDK II **基础库**：

```c
#include <Uefi.h>
// … 使用 EFI_SYSTEM_TABLE、ConOut 等已有抽象
```

| 对比 Ch 1 裸 C | EDK II 版 |
|----------------|-----------|
| 手动声明 `CHAR16`、协议结构体 | **`<Uefi.h>`** 统一类型与协议 |
| 单文件 + 简易 Makefile | **Package / Module / `.inf`** 工程化 |
| 实验程序 | 纳入 **MikanLoader** 与全书构建 |

**关键头文件：** `<Uefi.h>` — UEFI 规范中的基础类型、协议、服务表声明（来自 **MdePkg**）。

---

### 三、MikanLoader 命名

| 名称 | 含义 |
|------|------|
| **MikanLoader** | 「蜜柑加载器」— 本书 OS 的 **Boot Loader 雏形** |
| **当前能力** | Hello World + **内存映射导出**（本章） |
| **后续演进** | Ch 3 起加载内核、初始化硬件 — 仍属 Loader 职责 |

```
MikanLoader.efi（UEFI 阶段）
    ↓ 未来
MikanOS 内核（Ch 8+ 内存管理、Ch 19 分页…）
```

---

### 四、Boot Services vs Runtime Services（概念）

| 服务 | 阶段 | 本章常用 |
|------|------|----------|
| **Boot Services (`gBS`)** | OS 加载 **之前** | **GetMemoryMap**、内存分配、文件 I/O |
| **Runtime Services (`gRT`)** | OS 运行后仍可用部分 | 本章暂不深入 |

MikanLoader 运行在 **Boot Services 期** — 可调用 `gBS` 下各类协议。

→ 衔接 [Ch 1 EfiMain](../chapter-01-hello-world/notes/section-6-C语言过渡与文件格式.md)

---

### 五、全程 LLVM：EDK II 与 CLANGPDB

若 Ch1 已走 **Clang + ld.lld**，Ch2 可在 EDK II 里 **继续统一 LLVM**，不必切 GCC 交叉链。

| 配置 | 作用 |
|------|------|
| **`TOOL_CHAIN_TAG = CLANGPDB`** | MikanLoaderPkg / 平台 DSC 里指定 **Clang + LLD** 工具链 |
| **`build -t CLANGPDB`** | 命令行等价 — 构建产物目录常含 `CLANGPDB` 字样 |
| **前提** | 已 `source edksetup.sh`；PATH 有 **clang**、**ld.lld** |

**与 Ch1 关系：**

```
Ch1  hello.c  +  clang/ld.lld  →  BOOTX64.EFI（裸 C 最小模板）
Ch2  MikanLoader  +  EDK II + CLANGPDB  →  Loader.efi（<Uefi.h> 工程化）
```

→ Ch1 工具链 [§2](../chapter-01-hello-world/notes/section-2-二进制编辑器与BOOTX64.md)

---

### 口述巩固 · 自测

1. **EDK II 和 UEFI 规范什么关系？** — 规范是「接口文档」；EDK II 是 **参考实现 + 工具链 + 库**。
2. **MikanOS 用 EDK II 干什么？** — 编 **MikanLoader.efi** 等 UEFI 应用，不是要你重写整片主板 BIOS。
3. **OVMF 和 EDK II 关系？** — OVMF 是 **基于 EDK II 构建的 QEMU 用 UEFI 固件**。
4. **`.inf` / `.dsc` 在哪章查？** — [附录 C](../../appendix-C-edk2-files/)。

---

← [1. 本章定位](./section-1-本章定位.md) · [appendix-C](../../appendix-C-edk2-files/) · 下一节 [3. 内存映射](./section-3-主存储器与内存映射.md)
