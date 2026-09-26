# 01.7 · MSVC / Visual Studio：三条工具链的对照笔记

> **这本书回答一个问题：** 我在 Visual Studio 里点一下「运行」，中间到底发生了什么？
> 答案不在菜单里，在**工具链**里。VS 只是个壳，真正干活的是四个东西：
> **编译器 → 链接器 → 构建系统 → 调试器**。这四件套在你的 Mac（clang/lld/lldb/CMake）、
> 你的树莓派（gcc/binutils/gdb）、Windows（MSVC/MSBuild/link.exe）上**概念完全相同，只是拼写和文件格式不同**。
> 本模块把三套拼写摊在同一张表里，让你认出它们其实是同一个东西。

> **模块形态：** 工具链实战对照（非单一书目）
> **来源动机：** 新手在 VS 里写 C/C++ 很顺手，但不知道自己在用什么；一旦换平台就全废
> **核心方法：** 以 **MSVC 为"另一个世界的样本"**，用你已经有的 clang/CMake 去反衬它

---

## 先说三件会改变你判断的事

这三件是本模块实测/核实出来的事实，先看这三条再决定要不要往下读：

| # | 事实 | 后果 |
|---|------|------|
| 1 | **Visual Studio for Mac 已于 2024-08-31 退役**（微软 Modern Lifecycle 公告，[出处](https://learn.microsoft.com/lifecycle/announcements/visual-studio-mac-end-of-servicing)） | 在 Mac 上没有 VS 可用。微软官方给的建议是 VS Code 或虚拟机跑 Windows 版 VS。**VS ≠ VS Code**，两者品牌共用、架构完全无关 |
| 2 | 你 Mac 上 **Xcode.app 已装（27.0）**，但**许可协议未同意** → `xcrun --find clang/lldb/make` 全部报错 | 一条 `sudo xcodebuild -license` 就能解锁 Apple 官方整套工具链。详见 [3.3 Mac 实测](./03-debugger-mental-model/3.3-Mac实测lldb.md) |
| 3 | **MSVC 明确不打算支持 VLA**，官方文档原话是把它和 `gets()` 并列 | MSVC 不是"落后的 gcc"，它是一套**有意不同**的实现。包括 `<threads.h>`、`_Complex`、`aligned_alloc` 都缺 |

---

## 目录

| 章 | 标题 | 一句话 | 状态 |
|----|------|--------|------|
| **01** | [工具链解剖](./01-toolchain-anatomy/) | 先分清 VS/VS Code/VS for Mac 三个产品 → 拆开 `.sln` / `.vcxproj`，找出真正干活的四件套 | ⚠️ MSVC 侧成稿待实测 |
| **02** | [MSVC 编译器与选项](./02-msvc-compiler-and-options/) | `cl.exe` 长什么样；选项三 language 对照全表；版本号迷宫；CRT；调试信息三种容器 | ⚠️ MSVC 侧成稿待实测 |
| **03** | [调试器心智模型](./03-debugger-mental-model/) | **最实用的一章**：VS 的按钮 = lldb 的命令，本质同一件事 | ✅ **macOS 全程实测** |
| **04** | [Windows ↔ POSIX 边界](./04-windows-vs-posix/) | 为什么 TLPI 的代码在 VS 里连 `unistd.h` 都 include 不到 | ✅ 官方 documented 核实 |
| **05** | [速查表](./05-cheatsheet/) | 一页纸：三族编译器选项、调试器命令、常见坑 | — |
| **—** | [demo/](./demo/) | 一个 30 行的小程序，当所有调试器的靶子 | ✅ macOS 实测 |

> **状态约定：** ✅ 实测 = 在本仓库 `demo/` 上真实跑过，输出为真实终端输出。
> ⚠️ 成稿待实测 = 依官方文档/first-party 资料整理，**本机（macOS）无法执行 Windows 工具链**，
> 命令与选项按文档给出，未逐条复跑。等你回到 Windows 或有虚拟机后再改状态。

---

## 快速上手（5 分钟版）

打开 [第 3 章 调试器心智模型](./03-debugger-mental-model/)，跟着敲一遍 —— 那是唯一一章
能**立刻在你现在的 Mac 上跑起来**的：

```bash
cd demo
make                    # clang -g -O0 -Wall -Wextra -std=c11 ptrbug.c -o ptrbug
make lldb               # 非交互下一次断点跑一遍
```

如果你更想知道"VS 里那个绿色三角形到底做了什么"，从 [第 1 章](./01-toolchain-anatomy/) 开始顺序读。

---

## 与既有模块的边界

这个模块**不讲任何新知识体系**，它是把别处讲过的东西翻译成另一种方言：

| 维度 | 本模块 01.7 | [01.5 CMake](./../01.5-cmake-build/) | [03.6 用户态调试](./../03.6-userspace-debugging/) | [01 C 语言](./../01-c-language/) |
|------|-------------|----------------------|------------------------|------------------|
| 问的问题 | 这三套工具**怎么互译** | 怎么**描述**一个工程 | 程序**为什么坏了** | C 语法与指针本身 |
| 组织方式 | 工具链对照 | 任务节点式 | 问题类型驱动 | 书目一一对应 |
| 主角 | MSVC ↔ clang ↔ gcc | CMake 语法 | gdb / strace / ASan | 六本书 |

> **不重复原则：** 本模块不讲 CMake 语法（去 01.5）、不讲 gdb 用法明细（去 03.6）、
> 不讲 C 语法本身（去 01）。这里只做**横向互译**和**平台边界**。

---

## 三条工具链全景（先有个印象，后面逐项展开）

| 角色 | Windows (MSVC) | macOS (Apple/LLVM) | Linux (GNU) |
|------|----------------|---------------------|-------------|
| 编译器 | `cl.exe` | `clang` | `gcc` |
| 链接器 | `link.exe` | `ld64`（Apple）/ `lld` | `ld` (bfd/gold/lld) |
| 汇编器 | `ml64.exe`（很少手动用） | `clang -c`（集成） | `as` |
| 构建系统 | **MSBuild**（`.vcxproj`） | CMake/Makefile/Xcode | CMake/Makefile |
| 调试器 | VS 调试引擎 / `cdb` | `lldb` | `gdb` |
| 调试信息格式 | **PDB**（CodeView） | **DWARF**（含 dSYM 容器） | **DWARF** |
| 目标文件格式 | PE/COFF | Mach-O | ELF |
| C 运行时 | UCRT + `vcruntime140.dll` | `libSystem` | glibc |

> 一句话记法：**同一件事，三个世界。** 表里的每一行在本模块后面都会展开成一张对照表。
