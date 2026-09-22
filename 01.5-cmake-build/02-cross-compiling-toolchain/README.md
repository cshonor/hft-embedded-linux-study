# 第 2 章 · toolchain file：交叉编译怎么说

> **本节讲什么：** 第 1 章的 demo 是"在 Mac 上编给 Mac 跑"。这章回答下一个问题：
> **同一份 CMakeLists.txt，怎么编出能在 ARM 板子上跑的二进制？**
> 答案是一份几十行的 toolchain file——把 Makefile 里写死的那串交叉编译 flag 挪到它该在的地方。
> 读完能做的事：给 ARM Linux（树莓派）和 Cortex-M 裸机（STM32）各配一次交叉构建，
> 并看懂配置输出里每一行在说什么。
>
> 前置：只需第 1 章。不要求有板子——配置阶段的输出本机就能看。
> 说明：本章命令与输出按 CMake 3.16+ 标准格式整理，本机复跑路径附在每一步后面。

---

## 2.1 问题的起点：编译机和运行机不是同一台

第 1 章的隐含假设是**编译机 = 运行机**（本机编译本机跑）。嵌入式把这层纸捅破了：

```text
编译机（host）                运行机（target）
Mac / x86_64 Linux  ──编译──▶  树莓派 5（aarch64 Linux）
PC / x86_64         ──编译──▶  STM32（Cortex-M3 裸机，连 OS 都没有）
```

板子性能弱、没键盘没编辑器，没人想在板子上装编译器——**在强机器上编出弱机器的二进制，
这就是交叉编译（cross compiling）**。

Makefile 时代这件事的写法，在 `STM32-/labs/00` 见过完全体：

```makefile
CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m3 -mthumb -Wall -O2
LDFLAGS = -T stm32.ld --specs=nosys.specs
```

能工作，但三个老问题原样出现：平台写死、找库靠手拼、换个芯片改一堆。
CMake 的答案是：**这些信息不属于工程描述，属于"工具链描述"——单独抽成一个文件。**

## 2.2 第一性原理：交叉编译只比本机编译多三件事

剥掉术语，交叉编译需要回答的问题只有三个：

| 问题 | Makefile 里的答案 | CMake 里的答案 |
|---|---|---|
| ① 产出物给谁跑？ | 隐含在编译器名字里 | `CMAKE_SYSTEM_NAME` + `CMAKE_SYSTEM_PROCESSOR` |
| ② 编译器换谁？ | `CC = arm-none-eabi-gcc` | `CMAKE_C_COMPILER`（三元组前缀展开） |
| ③ 头文件/库去哪找？ | 手拼 `-I` `-L` `-isystem` | `CMAKE_SYSROOT` + `CMAKE_FIND_ROOT_PATH_MODE_*` |

**三元组（target triple）**是贯穿全章的词：`<架构>-<厂商>-<系统>[-<ABI>]`。
交叉工具链里每个工具都带这个前缀——`arm-linux-gnueabihf-gcc` 读作
"给 ARM 架构、跑 Linux、用 gnueabihf ABI 的 gcc"。两个最常用的：

| 三元组 | 目标 | 对应场景 |
|---|---|---|
| `arm-linux-gnueabihf` | ARM 32 位 + Linux + 硬浮点 ABI | 树莓派 Zero/2、ARM 工控板（aarch64 板子用 `aarch64-linux-gnu`） |
| `arm-none-eabi` | ARM 裸机，无 OS | STM32 等 Cortex-M 单片机 |

**toolchain file 就是把上面三件事写成 CMake 变量的一份 .cmake 文件**。
它不属于任何一个工程——同一套板子，所有工程共用一份。

## 2.3 一份 toolchain file 逐行解剖

`demo/arm-none-eabi.cmake`（Cortex-M3 裸机）全文逐段看：

```cmake
# ① 产出物给谁跑
set(CMAKE_SYSTEM_NAME Generic)        # 裸机 = Generic；跑 Linux 就写 Linux
set(CMAKE_SYSTEM_PROCESSOR arm)
```

`CMAKE_SYSTEM_NAME` 是**交叉编译的总开关**：只要它不等于本机系统，
CMake 就把 `CMAKE_CROSSCOMPILING` 置 ON，后续行为全部切换成交叉模式。
裸机写 `Generic`——没有操作系统，不是拼错。

```cmake
# ② 编译器换谁
set(triple arm-none-eabi)
set(CMAKE_C_COMPILER   ${triple}-gcc)
set(CMAKE_ASM_COMPILER ${triple}-gcc)

# 裸机没有启动文件和链接脚本就链不出可执行文件，
# CMake 的编译器自检默认试链接 → 必然失败。改成只验证"能编译"：
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

`CMAKE_TRY_COMPILE_TARGET_TYPE` 是裸机 toolchain file 的**灵魂一行**，
没有它第一次配置就报 "C compiler cannot build executables"——
不是工具链坏了，是 CMake 的自检方式在裸机上不成立。

```cmake
# ③ CPU 架构 flag：Makefile 里那串 -mcpu 的归宿
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
```

`_INIT` 后缀的含义：**CMake 第一次探测编译器时就把这些 flag 带上**，
之后所有 target 自动继承。对比在 CMakeLists 里给每个 target 手写
`target_compile_options(... -mcpu=cortex-m3)`——那是把工具链信息泄进了工程描述，
换一块板子要改所有 CMakeLists，正是要避免的事。

```cmake
# ④ find_* 的搜索边界（最容易翻车的地方，2.5 专节讲）
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

`demo/arm-linux-gnueabihf.cmake` 结构相同，差别只有三处：
`CMAKE_SYSTEM_NAME Linux`（有 OS）、不需要 `TRY_COMPILE_TARGET_TYPE`（能正常链接）、
多一个可选的 `CMAKE_SYSROOT`（目标系统的 `/` 在本机的镜像，2.5 讲）。

## 2.4 跑一次：工程描述一个字不改

第 1 章的 `demo/` 原样拿来（本章 `demo/` 就是同款三文件工程），
只多传一个 `-D` 参数：

```bash
cmake -B build-m3 -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake
cmake --build build-m3
```

配置阶段输出（关键行）：

```text
-- The C compiler identification is GNU 13.2.1
-- Detecting C compiler ABI info - done
-- Check for working C compiler: .../arm-none-eabi-gcc - skipped
-- Configuring done
-- Generating done
```

逐行读：编译器从本机 clang 换成了 `arm-none-eabi-gcc` ✓；
`Check for working ... - skipped` 正是 `TRY_COMPILE_TARGET_TYPE` 在起作用
（跳过链接自检）✓。构建完验证产出物真的是 ARM 机器码：

```bash
$ file build-m3/ma_feed
build-m3/ma_feed: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), ...
```

`file` 说 ARM ✓。回本机目录 `cmake -B build` 再配一次，同一份 CMakeLists
产出的又是本机 x86_64/arm64 二进制——**工程描述与工具链彻底解耦，这就是这章的全部目的**。

> **CMakeLists 里写 `set(CMAKE_C_COMPILER arm-none-eabi-gcc)` 为什么没用？**
> 老式教程常这么教。问题在时机：`project()` 命令在 CMakeLists **第 2 行**
> 就完成了编译器探测，第 10 行再 set 已经晚了。toolchain file 由
> `-DCMAKE_TOOLCHAIN_FILE` 在**第一次 `project()` 之前**注入，时机才对。
> 同理，`set(CMAKE_SYSTEM_NAME ...)` 写在 CMakeLists 里也来不及触发交叉模式。

## 2.5 交叉编译的找库边界：find_* 三组开关

工程一大就要 `find_library` / `find_package` 找第三方库。交叉编译时的经典事故：
**链接器报一堆 `undefined reference`，或更阴的——链上了本机 x86 的库，
板子上一跑就 `Exec format error`**。原因：find_* 默认搜本机路径。

CMake 用三组开关把搜索范围钉死（toolchain file 的第 ④ 段）：

| 开关 | 管什么 | 交叉时的正确值 | 为什么 |
|---|---|---|---|
| `..._MODE_PROGRAM` | `find_program`（protoc 等**在本机跑**的工具） | `NEVER` | 代码生成器要在编译机上执行，必须找本机版 |
| `..._MODE_LIBRARY` | `find_library` | `ONLY` | 库要链进目标二进制，只能用 sysroot 里的 ARM 版 |
| `..._MODE_INCLUDE` | `find_path`（头文件） | `ONLY` | 同理，头文件必须来自目标系统 |

配套概念 **`CMAKE_SYSROOT`**：把目标板根文件系统的 `/` 复制（或挂载）到本机一个目录，
里面 `usr/include`、`lib` 俱全，find_* 就进这个"镜像根"里找。
Buildroot/Yocto（08 模块）构建时会自动产出 sysroot，届时把路径填进 toolchain file 即可。

## 2.6 本章验收清单

- [ ] 能说清三元组三段各是什么，认出 `arm-linux-gnueabihf` 和 `arm-none-eabi` 的差别（有没有 OS）
- [ ] 知道 `CMAKE_SYSTEM_NAME` 是交叉模式总开关，裸机写 `Generic`
- [ ] 知道 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` 是治什么报的什么错
- [ ] 说得出为什么交叉信息要放 toolchain file 而不是 CMakeLists（时机 + 解耦两个理由）
- [ ] find_* 三组开关哪个 NEVER 哪个 ONLY，能说出一个搞反的事故场景

## 2.7 与后续衔接

- **第 3 章** 工程长大：一个 target 拆成"库 + 应用"的多目录结构——
  交叉编译的复杂度会乘以目录数，先把组织术学会
- **08-embedded-boot-build**：Buildroot 构建整个 rootfs 时，底层就在玩 sysroot 和三元组，
  本章概念到那边直接复用
- 小练习：给 `demo/` 不配 toolchain file 正常 `cmake -B build` 一次，
  再配 `arm-none-eabi.cmake` 一次，对比两个 `CMakeCache.txt` 里
  `CMAKE_C_COMPILER` 和 `CMAKE_SYSTEM_NAME` 的值——缓存是 CMake 的"日记"，会读它就会排障

## 代码自测

<details><summary>Q1：toolchain file 和 CMakeLists.txt 的分工是什么？为什么交叉信息不能写进 CMakeLists？</summary>

CMakeLists 是**工程描述**（有哪些 target、源码、依赖），平台无关，所有构建环境共用一份；
toolchain file 是**工具链描述**（编译器是谁、目标系统是谁、去哪找库），随编译环境走，
同一块板子的所有工程共用一份。不能写进 CMakeLists 的两个理由：①时机——`project()`
在 CMakeLists 开头就完成编译器探测，之后再 `set(CMAKE_C_COMPILER)` 已无效，
而 toolchain file 在首次 `project()` 之前注入；②解耦——写进 CMakeLists 意味着换一块板子
要改所有工程文件，交叉信息"漏"进了本该平台无关的工程描述。
</details>

<details><summary>Q2：裸机 toolchain file 里 set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) 是干什么的？</summary>

CMake 配置时会做一次编译器自检：默认试**链接**一个可执行文件来验证工具链可用。
裸机没有启动文件（startup code）和链接脚本，链接必然失败，自检误报
"C compiler cannot build executables"。把自检目标改成 STATIC_LIBRARY 后，
CMake 只验证"能否编译出目标文件"，跳过链接——自检通过，且符合裸机实际：
能不能链出固件是链接脚本齐备之后的事，不该由编译器自检背锅。
</details>

<details><summary>Q3：CMAKE_FIND_ROOT_PATH_MODE_PROGRAM 为什么是 NEVER 而 LIBRARY 是 ONLY？</summary>

两类东西的运行位置不同。`find_program` 找的是**在编译机上执行**的工具
（如 protoc、python——生成代码用的），必须在本机路径找，所以 NEVER 进 sysroot；
`find_library` / `find_path` 找的是**链进目标二进制**的库和头文件，
必须是目标架构的版本，所以 ONLY 在 sysroot 里找——放它去本机找，
就会链上 x86 的 .so，板子上 `Exec format error`。
口诀：**跑的工具找本机，链的库找目标。**
</details>

<details><summary>Q4：CMAKE_SYSROOT 是什么？没有它会怎样？</summary>

目标系统根目录在本机的镜像（含 usr/include、lib 等），交叉编译时编译器和
find_* 都进这个"影子根"找头文件和库。没有它：纯用户态小程序靠编译器自带的
头文件（stddef.h 等）往往也能编过，但一旦用到目标系统的库（如 libgpiod、
板子特有的驱动头）就找不到。Buildroot/Yocto 构建 rootfs 时会产出配套 sysroot，
届时把路径写进 toolchain file 的 CMAKE_SYSROOT 即可。
</details>

<details><summary>Q5：同一份代码要同时维护"本机调试版"和"ARM 发布版"，目录怎么组织？</summary>

两个 build 目录并存：`cmake -B build`（本机）+ `cmake -B build-arm
-DCMAKE_TOOLCHAIN_FILE=...`（ARM）。CMake 的 out-of-source 设计就是为这个场景：
每个 build 目录有独立的 CMakeCache.txt，记录各自的编译器和选项，互不污染；
源码目录始终只有一份。这也是 build/ 必须进 .gitignore 的原因之一——
本机缓存被带到另一台机器上会出"编译器路径不存在"的诡异错误。
</details>
