# 工程 02 · `<Uefi.h>` + GNU-EFI + GCC（Linux / WSL）

> **前提：** 建议先做 [01-clang-minimal](../01-clang-minimal/)，理解裸 C 版 `EfiMain` 和 `ConOut` 之后再来看本工程。

---

## 1. 要解决什么目标？

| 目标 | 说明 |
|------|------|
| **同样打印 Hello World** | 和工程 01 一样的 UEFI 效果，换一条工具链 |
| **用上标准 `<Uefi.h>`** | 不再手写结构体 — 用 gnu-efi 包提供的官方风格头文件 |
| **理解 gnu-efi 补了什么** | crt0 入口包装、PE 链接脚本、`libefi` — 仍 **无 EDK II 构建树** |
| **衔接 Ch2** | 习惯 `#include <Uefi.h>` 后，进 EDK II 的 `.inf/.dsc` 流程更自然 |

---

## 2. 思路是什么？（和工程 01「裸 C」差在哪？）

### 工程 01（裸 C）回顾

- `hello.c` **自己声明** `EFI_SYSTEM_TABLE`、`ConOut` 等
- 入口 **`EfiMain`**，链接器直接 `/entry:EfiMain`
- 只有 Clang + lld-link，零第三方 UEFI 包

### 本工程（GNU-EFI）

- 源码第一行：**`#include <Uefi.h>`** — 类型、宏、`EFI_SUCCESS` 等全在这里
- 入口改成 **`efi_main`** — gnu-efi 的 **crt0-efi-x86_64.o** 会先跑一小段启动代码，再调你的 `efi_main`
- **GCC** 编译 + gnu-efi 提供的 **链接参数** 产出 PE32+ `BOOTX64.EFI`

```
固件加载 BOOTX64.EFI
    → crt0（gnu-efi 提供）设置栈、调用约定
    → 你的 efi_main(ImageHandle, SystemTable)
    → ConOut->OutputString(...)
    → return EFI_SUCCESS
```

**仍不是 EDK II：** 没有 `.inf` / `.dsc` / `build` 命令，只是 apt 装一个小包 `gnu-efi`。

→ 笔记：[§7 阶段 A2 · GNU-EFI](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 3. 一步一步怎么做

### 步骤 0 · 准备环境（仅 Linux / WSL）

```bash
sudo apt install gnu-efi gcc-multilib x86_64-linux-gnu-gcc make qemu-system-x86 ovmf
```

**Windows 原生不适用** — gnu-efi 是 Linux 包；Windows 请用 [01-clang-minimal](../01-clang-minimal/)。

### 步骤 1 · 读源码 `main.c`

对比 [01 的 hello.c](../01-clang-minimal/hello.c)：

```c
#include <Uefi.h>          // ← 工程 01 没有这一行

EFI_STATUS EFIAPI efi_main(   // ← 工程 01 叫 EfiMain
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
  SystemTable->ConOut->OutputString(
      SystemTable->ConOut,
      L"Bare C BOOTX64.EFI Hello World!\r\n");
  return EFI_SUCCESS;       // ← 工程 01 写 return 0
}
```

| 对比项 | 01 裸 C | 02 本工程 |
|--------|---------|-----------|
| 头文件 | 手写结构体 | `<Uefi.h>` |
| 入口 | `EfiMain` | `efi_main` |
| 返回值 | `0` | `EFI_SUCCESS` |
| 换行 | `\n` | `\r\n`（UEFI 控制台惯例） |

### 步骤 2 · 编译 + 链接

```bash
cd chapter-01-hello-world/code/02-gnu-efi-gcc
make
```

Makefile 内部大致做：

1. `x86_64-linux-gnu-gcc -c main.c` → `main.o`
2. 链接 `main.o` + gnu-efi 的 **crt0** + **libefi**，输出 **PE32+** 格式的 `BOOTX64.EFI`

### 步骤 3 · 摆 FAT 布局并运行

```bash
make run
```

等价于：把 `BOOTX64.EFI` 复制到 `ESP/EFI/BOOT/`，再启动 QEMU + OVMF。

**预期结果：** 控制台出现 `Bare C BOOTX64.EFI Hello World!`

### 步骤 4 · 清理

```bash
make clean
```

---

## 4. 目录里每个文件是什么

| 文件 | 角色 |
|------|------|
| **`main.c`** | 源码 — `#include <Uefi.h>` |
| **`Makefile`** | 调用 gcc + gnu-efi 链接规则 |
| **`main.o`** | 编译中间产物（make 后生成） |
| **`BOOTX64.EFI`** | 链接产物（make 后生成） |
| **`ESP/`** | `make esp` / `make run` 生成的 FAT 布局 |

---

## 5. 三条路线对照（帮助记忆）

| | **01 裸 C** | **02 本工程** | **Ch2 EDK II** |
|---|-------------|---------------|----------------|
| `<Uefi.h>` | ❌ 手写 | ✅ gnu-efi 包 | ✅ EDK 完整版 |
| 构建 | clang + lld-link | gcc + gnu-efi | `build` + `.inf/.dsc` |
| 入口 | `EfiMain` | `efi_main` | 模块入口（由 EDK 生成） |
| Windows | ✅ | ❌ | WSL 为主 |

---

## 6. 下一步

- 读 [§7 全链路](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md) 阶段 B — EDK II **MikanLoader**
- 进 [Ch2](../../../chapter-02-edk2-memmap/)

← 返回 [code/ 总索引](../README.md)
