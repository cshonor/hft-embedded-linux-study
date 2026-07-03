# Ch1 · 代码工程索引

> **先看这里，再进子目录。** 第一章有两个独立小工程，各编各的 `BOOTX64.EFI`。

---

## 三条路线（递进关系）

MikanOS 第一章到第二章，UEFI 程序有三种写法，**难度递增、依赖递增**：

```
① 裸 C（本仓库默认）     ② GNU-EFI + <Uefi.h>      ③ EDK II（Ch2 起）
   01-clang-minimal          02-gnu-efi-gcc              chapter-02 MikanLoader
   手写最少类型               apt 装 gnu-efi              .inf / .dsc + build
   无 Uefi.h                  真 #include <Uefi.h>        完整 Uefi.h + 官方库
```

| 阶段 | 头文件 | 构建 | 本章工程 |
|------|--------|------|----------|
| **裸 C** | **不** `#include <Uefi.h>`，只声明用到的结构体 | Clang + lld-link | **[01-clang-minimal/](./01-clang-minimal/)** |
| **GNU-EFI** | **`#include <Uefi.h>`**（gnu-efi 包提供） | GCC + gnu-efi 链接脚本 | **[02-gnu-efi-gcc/](./02-gnu-efi-gcc/)** |
| **EDK II** | 完整 **`<Uefi.h>`** + BaseLib 等 | `build` + `.inf/.dsc` | **Ch2** [MikanLoader](../../chapter-02-edk2-memmap/) |

**你记得的 `<Uefi.h>` 没错** — 那是 **② 和 ③** 用的标准头文件。  
**工程 01 故意不用它**，先把「固件怎么调你的 C 函数、怎么打印一行字」看清楚；看完再进 02 或 Ch2。

→ 理论对照：[§7 裸 C → EDK II 全链路](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)

---

## 选哪个工程？

| 工程 | 目录 | 何时用 |
|------|------|--------|
| **01 · 裸 C + Clang** | **[01-clang-minimal/](./01-clang-minimal/)** | **默认、先做。** Windows 已装 LLVM，或 WSL 想最快跑通 |
| **02 · `<Uefi.h>` + GCC** | **[02-gnu-efi-gcc/](./02-gnu-efi-gcc/)** | 裸 C 跑通后，在 Linux/WSL 体验 **真 Uefi.h** 和 gnu-efi 链接 |

---

## 推荐学习顺序

1. 读 **[01-clang-minimal/README.md](./01-clang-minimal/README.md)** — 目标、思路、逐步编译
2. 编出 `BOOTX64.EFI`，QEMU 看到 `Hello, world!`
3. （可选 WSL）读 **[02-gnu-efi-gcc/README.md](./02-gnu-efi-gcc/README.md)** — 对比 `#include <Uefi.h>` 差在哪
4. 读笔记 [§7](../notes/section-7-Ch1裸C与Ch2-EDKII全链路.md)，进 **Ch2 EDK II**

---

## 常见困惑

| 你看到的 | 是什么 |
|----------|--------|
| **`hello.c` / `main.c`** | 源码 — 在对应**工程子目录**里，不在 `code/` 根目录 |
| **`hello.o` / `main.o`** | 编译中间产物 — `make` 或 `clang -c` 后才有 |
| **`esp/` / `ESP/`** | FAT 启动盘布局 — 内含 `EFI/BOOT/BOOTX64.EFI` |
| **`BOOTX64.EFI`** | 最终 UEFI 程序 — 固件从 FAT 加载的 PE 文件 |

**不要在 `code/` 根目录找 Makefile** — 请进入 `01-clang-minimal/` 或 `02-gnu-efi-gcc/`。

→ 环境安装 [SETUP.md](../../SETUP.md) · 理论 [notes/](../notes/)
