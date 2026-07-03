## 7. UEFI 开发两阶段：Ch1 裸 C → Ch2 EDK II 全链路

> **递进逻辑：** Ch1 **剥离框架** 看懂 UEFI 入口 / PE32+ / 系统表；Ch2 **工业标准化** — `.inf/.dsc/.dec` + BaseTools + 官方库生态。  
> 本仓库 Ch1 **默认**：[01-clang-minimal](../code/01-clang-minimal/)（WSL · Clang + ld.lld）。

---

### 一、总览

```
阶段 A  裸 C 生成 BOOTX64.EFI（无 EDK II 构建树）
          ├─ A1 极简：手写类型 + Clang/ld.lld     [01-clang-minimal/](../code/01-clang-minimal/)  WSL
          └─ A2 GNU-EFI：真 <Uefi.h> + GCC 链接    [02-gnu-efi-gcc/](../code/02-gnu-efi-gcc/)  WSL

阶段 B  EDK II 工程化（Ch2 MikanLoader 及以后）
          .c + .inf + .dsc + build → Loader.efi
```

---

## 阶段 A · Ch1 裸 C BOOTX64.EFI

### A1. 本仓库默认（极简 · Clang）

- **无** EDK II、**无** gnu-efi — 只声明本章用到的结构体
- WSL：`clang --target=x86_64-elf …` + `ld.lld -flavor link` → [SETUP.md](../../SETUP.md)
- **目的：** 最快跑通 + 理解 **PE / EfiMain / ConOut**

### A2. GNU-EFI + GCC（真 `<Uefi.h>` · Linux）

#### 原理

不引入 EDK II 庞大构建体系；靠 **gnu-efi** 提供：

- 标准 **`Uefi.h`**
- **crt0-efi-x86_64.o**（入口包装）
- **libefi / libgnuefi**

用 **GCC** 直接链接出 **PE32+** `BOOTX64.EFI` — 适合理解 **UEFI ABI、PE 结构、固件调用规范**。

#### 环境

```bash
sudo apt install gnu-efi gcc-multilib x86_64-linux-gnu-gcc make qemu-system-x86 ovmf
```

#### 源码 · 入口 `efi_main`

```c
#include <Uefi.h>

EFI_STATUS EFIAPI efi_main(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    SystemTable->ConOut->OutputString(
        SystemTable->ConOut,
        L"Bare C BOOTX64.EFI Hello World!\r\n");
    return EFI_SUCCESS;
}
```

#### 构建与运行

```bash
cd chapter-01-hello-world/code/02-gnu-efi-gcc
make run    # BOOTX64.EFI → ESP/EFI/BOOT/ → QEMU+OVMF
```

→ 完整 Makefile：[02-gnu-efi-gcc/](../code/02-gnu-efi-gcc/)

#### 裸 C 方案（A1+A2）优缺点

| 优势 | 缺陷 |
|------|------|
| 零 EDK 构建脚本学习成本 | 无 **Print** / 内存 / 协议 **官方库封装** |
| 直观看懂入口、系统表、PE 链接 | 无 **.inf/.dsc** 模块化管理 |
| 适合原理入门、简易 Demo 引导器 | 无 PCD / Secure Boot 完整支持；**不能** 开发整片主板固件 |

---

## 阶段 B · Ch2 EDK II 标准化工程

### 1. 核心文件分工（一条编译链）

| 文件 | 作用 | 层级 |
|------|------|------|
| **`.c`** | 业务逻辑，含 EDK **`<Uefi.h>`** | 模块 |
| **`.inf`** | 单模块：入口、源文件、依赖库、Packages | 模块配置 |
| **`.dec`** | Package 声明：头文件/GUID/PCD（**MdePkg.dec** 为基础） | 包 |
| **`.dsc`** | 平台顶层：架构、工具链、注册所有 `.inf` | 平台 |
| **BaseTools** | AutoGen、GenFw、GenFds 等 — 生成标准 `.efi` | 构建工具 |

```
.dsc → 选模块 + 工具链
  → .inf → 编每个 Module
  → .dec → 导出头文件 / GUID
  → build → Build/.../BareDemo.efi
```

→ 模板详表 [appendix-C](../../appendix-C-edk2-files/notes/section-附录C-待补充.md)

### 2. 工程目录（示例 MyPkg）

```
edk2/
└── MyPkg/
    ├── MyPkg.dsc
    ├── MyPkg.dec          # 简易工程可主要依赖 MdePkg.dec
    └── Application/
        └── BareDemo/
            ├── BareDemo.c
            └── BareDemo.inf
```

MikanOS 书中对应 **MikanLoaderPkg** — 链入 edk2 树后 `build`。

### 3. EDK 标准源码（BareDemo.c）

```c
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

EFI_STATUS EFIAPI UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    Print(L"EDK II Standard BOOTX64.EFI Demo\r\n");
    return EFI_SUCCESS;
}
```

| 对比 GNU-EFI 裸 C | EDK II |
|-------------------|--------|
| 手写 `ConOut->OutputString` | **`Print(L"...")`** 库封装 |
| 入口 **`efi_main`** | 入口 **`UefiMain`** + `UefiApplicationEntryPoint` 库 |

### 4. 标准构建流程

```bash
cd edk2
git submodule update --init
make -C BaseTools
source edksetup.sh
```

**Conf/target.txt（示例）**

```
ACTIVE_PLATFORM       = MyPkg/MyPkg.dsc
TARGET_ARCH           = X64
TOOL_CHAIN_TAG        = GCC5
# 或 CLANGPDB — 与 Ch1 LLVM 路线统一
```

```bash
build
# 产物：Build/MyPkg/DEBUG_GCC5/X64/BareDemo.efi
# 复制为 ESP/EFI/BOOT/BOOTX64.EFI 即可引导
```

### 5. EDK 构建流水线（底层 6 步）

1. **依赖解析** — 读 `.dsc/.inf/.dec`，匹配头文件、库、GUID  
2. **AutoGen** — 生成 `AutoGen.h`、入口包装、PCD、协议全局变量  
3. **编译** — 业务 `.c` + AutoGen → `.o`  
4. **链接 PE DLL** — 合并库 → 中间 DLL  
5. **GenFw** — 修正 PE 头，输出标准 **`.efi`**  
6. **GenFds**（固件扩展）— 多 EFI 打包为 FV → 整机 **`.fd`** BIOS 镜像  

### 6. EDK II 相对裸 C 的优势

1. **标准库生态** — 控制台、文件、PCIe、TPM2、ACPI、网络  
2. **模块化** — 多驱动/多应用统一管理  
3. **PCD** — 全局配置，少改源码  
4. **Secure Boot / 测量启动** 原生支持  
5. **跨架构** — X64 / AArch64 / RISC-V / LoongArch  
6. **调试** — DEBUG 日志、GDB、OVMF 深度适配  

---

### 三、三路径对照

| | **A1 极简 Clang** | **A2 GNU-EFI** | **B EDK II** |
|---|-------------------|----------------|--------------|
| **Uefi.h** | 手写最小类型 | 包内真头文件 | **MdePkg** |
| **入口** | `EfiMain` | `efi_main` | `UefiMain` |
| **工具** | clang + ld.lld | gcc + gnu-efi | `build` + BaseTools |
| **平台** | **WSL / Linux** | WSL / Linux | WSL / Linux（全书统一） |
| **本书章节** | Ch1 [01-clang-minimal](../code/01-clang-minimal/) | Ch1 [02-gnu-efi-gcc](../code/02-gnu-efi-gcc/) | **Ch2+** MikanLoader |

---

### 四、递进结论

1. **Ch1 裸 C：** 剥离框架，理解 **Uefi.h 语义、入口 ABI、PE32+ 链接** — **`GetMemoryMap` 等 API 来自主板固件，不来自 EDK II**。  
2. **Ch2 EDK II：** 工业标准 **工程化** MikanLoader — **`.inf/.dsc/.dec`、MdePkg 库、AutoGen/GenFw**；量产 BIOS / OVMF 的官方开发方式。  
3. **不必二选一误解：** 吃透原理 **不必先学完整 EDK II**；做整机 BIOS **才必须** EDK II 全栈。

### 口述巩固 · 自测

1. **EDK II 只是 C 库吗？** — **整体不是**；**`MdePkg` 一块** 可视为 UEFI C 库。  
2. **`GetMemoryMap` 谁提供的？** — **主板 UEFI 固件**；EDK II 只提供 **头文件/封装**。  
3. **GNU-EFI 和 EDK II 都提供 Uefi.h，区别？** — gnu-efi 是 **小包装+GCC 链接**；EDK II 是 **完整构建树+库生态+可产整板固件**。

---

← [§6 C 与 PE](./section-6-C语言过渡与文件格式.md) · [Ch2 §2 索引](../../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md) · [appendix-C](../../appendix-C-edk2-files/)
