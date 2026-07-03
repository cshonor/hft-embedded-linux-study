# 工程 02 · `<Uefi.h>` + GNU-EFI + GCC（WSL）

> **前提：** 建议先做 [01-clang-minimal](../01-clang-minimal/)，理解裸 C 版 `EfiMain` 和 `ConOut` 之后再来看本工程。  
> **环境：** 与工程 01 相同 — **WSL2 + Ubuntu**（见 [SETUP.md](../../../SETUP.md)）。

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
- 入口 **`EfiMain`**，链接器 `-entry:EfiMain`
- Clang + **ld.lld**，零第三方 UEFI 包

### 本工程（GNU-EFI）

- 源码第一行：**`#include <Uefi.h>`**
- 入口 **`efi_main`** — gnu-efi 的 **crt0-efi-x86_64.o** 先跑启动代码，再调你的 `efi_main`
- **GCC** 编译 + gnu-efi 链接参数产出 PE32+ `BOOTX64.EFI`

→ 笔记：[§7 阶段 A2 · GNU-EFI](../../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 3. 一步一步怎么做（WSL）

### 步骤 0 · 安装依赖

```bash
sudo apt install gnu-efi gcc-multilib x86_64-linux-gnu-gcc make qemu-system-x86 ovmf
```

### 步骤 1 · 读源码 `main.c`

对比 [01 的 hello.c](../01-clang-minimal/hello.c)：`#include <Uefi.h>`、入口 `efi_main`、`EFI_SUCCESS`。

### 步骤 2 · 编译 + 链接 + 运行

```bash
cd chapter-01-hello-world/code/02-gnu-efi-gcc
make run
make clean
```

---

## 4. 三条路线对照

| | **01 裸 C** | **02 本工程** | **Ch2 EDK II** |
|---|-------------|---------------|----------------|
| `<Uefi.h>` | ❌ 手写 | ✅ gnu-efi 包 | ✅ EDK 完整版 |
| 构建 | clang + ld.lld | gcc + gnu-efi | `build` + `.inf/.dsc` |
| 入口 | `EfiMain` | `efi_main` | `UefiMain` |

← 返回 [code/ 总索引](../README.md)
