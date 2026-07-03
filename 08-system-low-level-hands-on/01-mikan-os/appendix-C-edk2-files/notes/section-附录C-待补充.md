# 附录 C · EDK II 文件说明

> **定位：** `.inf` / `.dec` / `.dsc` 模板与构建链 — 配合 [Ch1 §7 两阶段全链路](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) · [Ch2 §2](../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md)。

---

## 1. 三种核心元文件

| 文件 | 角色 | 层级 |
|------|------|------|
| **`.dec`** | Package **声明** — 导出头文件、GUID、PCD | 包 |
| **`.inf`** | **Module** — 单程序编译单元：源文件、库、入口 | 模块 |
| **`.dsc`** | **Platform** — 全局工具链、LibraryClasses、注册 `.inf` | 平台顶层 |

**BaseTools：** AutoGen → 编译 → 链接 → **GenFw** → `.efi`（固件开发再 **GenFds** → `.fd`）。

---

## 2. 示例工程 MyPkg（最小模板）

### 目录

```
edk2/MyPkg/
├── MyPkg.dsc
├── MyPkg.dec              # 简易工程可主要依赖 MdePkg.dec
└── Application/BareDemo/
    ├── BareDemo.c
    └── BareDemo.inf
```

### BareDemo.inf

```inf
[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = BareDemo
  FILE_GUID                      = 12345678-ABCD-4EF0-1234-567890ABCDEF
  MODULE_TYPE                    = UEFI_APPLICATION
  VERSION_STRING                 = 1.0
  ENTRY_POINT                    = UefiMain

[Sources]
  BareDemo.c

[Packages]
  MdePkg/MdePkg.dec

[LibraryClasses]
  UefiApplicationEntryPoint
  UefiLib
  UefiBootServicesTableLib
  BaseLib
  BaseMemoryLib
```

### MyPkg.dsc（摘录）

```dsc
[Defines]
  PLATFORM_NAME                  = MyPkg
  PLATFORM_GUID                  = 87654321-DCBA-4FE0-9876-FEDCBA098765
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/MyPkg
  SUPPORTED_ARCHITECTURES        = X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

[Components]
  MyPkg/Application/BareDemo/BareDemo.inf
```

---

## 3. 构建命令

```bash
cd edk2 && git submodule update --init
make -C BaseTools
source edksetup.sh
# Conf/target.txt: ACTIVE_PLATFORM = MyPkg/MyPkg.dsc
build
# → Build/MyPkg/DEBUG_GCC5/X64/BareDemo.efi
```

**LLVM 工具链：** `TOOL_CHAIN_TAG = CLANGPDB` → [Ch2 §5 CLANGPDB](../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md#五全程-llvmedk-ii-与-clangpdb)

---

## 4. MikanOS 对应

| 示例 | 本书 |
|------|------|
| `MyPkg` | **MikanLoaderPkg** |
| `BareDemo.efi` | **Loader.efi** / MikanLoader |

→ [mikanos-build](https://github.com/uchan-nos/mikanos-build) · [SETUP.md](../SETUP.md)

---

← [Ch1 §7 全链路](../chapter-01-hello-world/notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) · [Ch2 §2](../chapter-02-edk2-memmap/notes/section-2-EDK-II与MikanLoader.md)
