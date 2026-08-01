## 2.2 用 EDK II 重写 Hello World

> **§2 子笔记 2/5** · [§2 索引](./section-2-EDK-II与MikanLoader.md)

---

Ch1 已用 C + Clang/**lld-link** 写出 Hello World；本章改用 EDK II **基础库**：

**本仓库源码：** [code/MikanLoaderPkg/](../code/MikanLoaderPkg/) · 构建步骤 [code/README.md](../code/README.md)（WSL / Linux）

```c
#include <Uefi.h>
// … 使用 EFI_SYSTEM_TABLE、ConOut 等已有抽象
```

| 对比 Ch1 裸 C | EDK II 版 |
|----------------|-----------|
| 手动声明 `CHAR16`、协议结构体 | **`<Uefi.h>`** 统一类型与协议 |
| 单文件 + 简易 Makefile | **Package / Module / `.inf`** 工程化 |
| 实验程序 | 纳入 **MikanLoader** 与全书构建 |

**关键头文件：** `<Uefi.h>` — UEFI 规范中的基础类型、协议、服务表声明（来自 **MdePkg**）。

**构建产物：** 仍是 **PE32+ `.efi`**，仍由 UEFI 固件从 FAT 加载 —— 变的是 **源码组织与库**，不是启动链本身。

→ 源码：[Main.c](../code/MikanLoaderPkg/Main.c) · Ch1 对照：[§2 二进制与 BOOTX64](../../chapter-01-hello-world/notes/section-2-二进制编辑器与BOOTX64.md) · [01-clang-minimal](../../chapter-01-hello-world/code/01-clang-minimal/)  
→ 元文件：[appendix-C `.inf` / `.dsc`](../../appendix-C-edk2-files/)

---

← [2.1 EDK II 是什么](./section-2-1-EDK-II是什么与行业定位.md) · [§2 索引](./section-2-EDK-II与MikanLoader.md) · 下一篇 [2.3 MikanLoader 是什么](./section-2-3-MikanLoader是什么.md)
