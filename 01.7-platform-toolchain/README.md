# 01.7 · 三条 C 工具链：Windows / macOS / Linux 互译笔记

> **这本书回答一个问题：** 我在 Visual Studio 里点一下「运行」，中间到底发生了什么？
> 答案不在菜单里，在**工具链**里。VS 只是个壳，真正干活的是四个东西：
> **编译器 → 链接器 → 构建系统 → 调试器**。这四件套在你的 Windows（MSVC/MSBuild/link.exe）、
> 你的 Mac（Apple clang/ld64/lldb）、你的树莓派（gcc/binutils/gdb）上
> **概念完全相同，只是拼写和文件格式不同**。
> 本模块把三套拼写摊在同一张表里，让你认出它们其实是同一个东西。

> **模块形态：** 工具链实战对照（非单一书目）
> **来源动机：** 新手在 VS 里写 C/C++ 很顺手，但不知道自己在用什么；一旦换平台就全废
> **核心方法：** 三个平台各写一章，每章都用**同一份 `demo/ptrbug.c`** 当靶子 ——
> 同一段 bug 在四套编译器身上的不同命运，是理解工具链最快的路

---

## 模块结构：一个公共层 + 三条支线

```
01-common-core/     ← 平台无关的"心智模型"：什么是编译器，调试在做哪四件事
        │
        ├── 02-windows-msvc/   ← Visual Studio + MSVC + PDB（够不着，成稿待实测）
        ├── 03-macos-xcode/    ← Xcode + Apple clang + lldb + dSYM（✅ 本机实测）
        │
        │   两条支线都敲同一份 ptrbug.c，于是可以做对照实验 ↓
        │
        ├── 04-linux-gnu/      ← gcc + binutils + gdb + ELF（✅ Pi 实测）
        │
        └── 05-boundary/       ← 三平台的"不可通约"部分：为什么 TLPI 只能在 Linux 跑
```

| 章 | 支线 | 一句话 | 状态 |
|----|------|--------|------|
| **[CH1](./01-common-core/)** | 公共认知层 | 分清 VS / VS Code / VS for Mac；拆开 `.sln`/`.vcxproj`；认出编译器、链接器、构建系统、调试器；诊断调试在做哪四件事；三平台调试命令对照 | ✅ 概念 + 本机核实 |
| **[CH2](./02-windows-msvc/)** | **Windows / MSVC** | `cl.exe` 长什么样；选项三 language 对照全表；`_MSC_VER` 版本号迷宫；CRT 选型；PDB 与其他两家的调试信息容器 | ⚠️ 成稿待实测 |
| **[CH3](./03-macos-xcode/)** | **macOS / Xcode** | Xcode 三层结构与被许可卡住的 shim；Apple clang vs LLVM clang；lldb 全程实测；Mach-O 与 dSYM 分离税 | ✅ **本机实测** |
| **[CH4](./04-linux-gnu/)** | **Linux / GNU** | gcc 其实只是"司机"；`-fanalyzer` 也会沉默；gdb 全程实测；DWARF 就住在 ELF 里；Linux 上写 C 用什么 | ✅ **Pi 实测** |
| **[CH5](./05-boundary/)** | 平台边界 | MSVC 的 C 标准缺口、`<unistd.h>` 与 Winsock、UTF-8/CRLF、为什么 TLPI 必须在 Linux 上跑、**MSVC 不吃 GNU 扩展怎么办**、**内核模块要真 Linux 吗（WSL2 够不够 + Pi insmod 实测）** | ✅ 官方核实 + Pi 实测 |
| **[CH6](./06-cheatsheet/)** | 速查表 | 一页纸：三族选项映射、调试命令、常见坑 | — |
| **—** | [demo/](./demo/) | 60 行的 `ptrbug.c`，当四套编译器 + 三个调试器的共同靶子；另有 [`hello-mod/`](./demo/hello-mod/) 最小可加载内核模块 | ✅ Mac + Pi |

> **状态约定：** ✅ 实测 = 在 `demo/` 上真实跑过，输出为真机输出。
> ⚠️ 成稿待实测 = 依官方文档整理；Windows 工具链**本机与 Pi 均无法执行**，未逐条复跑。

---

## 先看这五条（全部为实测或官方核实）

| # | 事实 | 出处 | 后果 |
|---|------|------|------|
| 1 | **Visual Studio for Mac 已于 2024-08-31 退役** | 微软 Modern Lifecycle [公告](https://learn.microsoft.com/lifecycle/announcements/visual-studio-mac-end-of-servicing) | 你在 Mac 上搜到的 "Visual Studio" 引导你去装的是 **VS Code** —— 同名不同物的另一个产品 |
| 2 | **Mac 上没有 GNU gcc** | 本机实测：`/usr/bin/gcc` 与 `/usr/bin/clang` 是同一个 200560 字节的 shim | `gcc --version` 打印 clang 版本号不是 bug，是设计 |
| 3 | 这台 Mac 的 **Xcode 27.0 许可未同意** → shim 与 `xcrun` 全线报错 | 本机实测 | 绕过法：直接点名 Xcode 内的 clang 二进制 + `-isysroot`；根治法：你本人跑 `sudo xcodebuild -license` |
| 4 | **同一份 UB 源码，四个编译器给出三种答案** | Mac + Pi 实测（见 [3.2](./03-macos-xcode/3.2-两套clang与版本矩阵.md)） | clang 23 `-O2` 生成死循环卡死；Apple clang 21 / clang 19 / gcc 14 都正常退出。**UB 的后果由「编译器+版本+优化级别+平台」四者共同决定** |

---

## 核心实验：一份 bug，四套编译器

`demo/ptrbug.c` 故意用 `sum_n(buf, 8)` 去读只有 4 个元素的数组。
`-O0` 下四套编译器**全都输出正常、exit=0**。把优化开到 `-O2` 就分化了：

| 编译器 | 平台 | `-O0` | `-O2` |
|---|---|---|---|
| LLVM clang **23.1.0**（micromamba `cdev`） | macOS arm64 | 正常 | ❌ **永久卡住**（`main` 里只剩 `b LBB0_1` 自我跳转） |
| Apple clang **21.0.0**（Xcode 27） | macOS arm64 | 正常 | ✅ 正常退出 |
| clang **19.1.7**（Debian） | Linux aarch64 (Pi) | 正常 | ✅ 正常退出 |
| gcc **14.2.0**（Debian） | Linux aarch64 (Pi) | 正常 | ✅ 正常退出 |

> ① 与 ② 的差异**只来自编译器版本**（同一台机器、同一个 CPU、同一个 OS），
> 所以"-O2 会生成死循环"不是 macOS 的锅，是 LLVM 23 更激进的 UB 推断。
> 这一条也修正了本模块初版（commit `ba7f13bc0`）的过度概括。

而真正让人不安的是另一半：**`-Wall -Wextra` 沉默，gcc 14 的 `-fanalyzer` 也沉默**（[4.2](./04-linux-gnu/4.2-三道防线全部沉默.md)）。
只有 ASan 一句话抓现行。

---

## 三平台全景表（后面每一列都会展开成一张对照表）

| 角色 | Windows (MSVC) | macOS (Apple) | Linux (GNU) |
|---|---|---|---|
| 编译器 | `cl.exe` | Apple clang / clang | `gcc` / `clang` |
| 链接器 | `link.exe` | `ld64` | `ld`（GNU Binutils） |
| 汇编器 | `ml64.exe` | 集成在 clang 里 | `as` |
| 构建系统 | **MSBuild**（`.vcxproj`） | CMake / Makefile / Xcode | CMake / Makefile |
| 调试器 | VS 调试引擎 / `cdb` | `lldb` | `gdb` |
| 调试信息格式 | **PDB**（CodeView） | **DWARF**（放在 `.dSYM/`） | **DWARF**（放在 ELF 内） |
| `-g` 后本体变大吗 | 基本不变 | **基本不变**（+408 B） | **会变大**（+1672 B） |
| 目标文件格式 | PE/COFF | Mach-O | ELF |
| C 运行时 | UCRT + `vcruntime140.dll` | `libSystem` | glibc（2.41） |
| 查看二进制信息 | `dumpbin` | `otool` / `llvm-*` | `readelf` / `objdump` |

> 一句话记法：**同一件事，三个世界。**

---

## 快速上手

**在 Linux（你的 Pi）上 —— 最完整的一条路：**

```bash
scp demo/ptrbug.c wzp@192.168.31.109:~/demo-msvc/
ssh wzp@192.168.31.109 'cd ~/demo-msvc && gcc -g -O0 -Wall -Wextra -std=c11 ptrbug.c -o p && gdb -batch -ex "break ptrbug.c:34" -ex "run" -ex "info locals" -ex "backtrace" --args ./p'
```

**在 Mac 上 —— 用 cdev 那套 clang：**

```bash
export PATH="/Users/a0000/micromamba/envs/cdev/bin:$PATH"
cd demo && make          # 编译
cd demo && make lldb     # 非交互跑一次断点（需 Xcode 的 lldb，见 3.3）
```

**Windows 那一章只能读不能跑** —— 这是本模块唯一标 ⚠️ 的部分。

---

## 与既有模块的边界

这个模块**不讲任何新知识体系**，它是把别处讲过的东西翻译成另外两种方言：

| 维度 | 本模块 01.7 | [01.5 CMake](../01.5-cmake-build/) | [03.6 用户态调试](../03.6-userspace-debugging/) | [01 C 语言](../01-c-language/) |
|------|-------------|----------------------|------------------------|------------------|
| 问的问题 | 这三套工具**怎么互译** | 怎么**描述**一个工程 | 程序**为什么坏了** | C 语法与指针本身 |
| 组织方式 | 平台分章 | 任务节点式 | 问题类型驱动 | 书目一一对应 |
| 主角 | MSVC ↔ clang ↔ gcc | CMake 语法 | gdb / strace / ASan | 六本书 |

> **不重复原则：** 本模块不讲 CMake 语法（去 01.5）、不讲 gdb 用法明细（去 03.6）、
> 不讲 C 语法本身（去 01）。这里只做**横向互译**与**平台边界**。
