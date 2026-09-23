# 第 2 章 · toolchain file：交叉编译怎么说

> **本章讲什么：** [第 1 章](../01-minimal-c-project/README.md)的 demo 是"在 Mac 上编给 Mac 跑"。这章回答下一个问题：
> **同一份 CMakeLists.txt，怎么编出能在 ARM 板子上跑的二进制？**
> 答案是一份几十行的 toolchain file——把 Makefile 里写死的那串交叉编译 flag 挪到它该在的地方。
> 读完能做的事：给 ARM Linux（树莓派）和 Cortex-M 裸机（STM32）各配一次交叉构建，
> 并看懂配置输出里每一行在说什么。
>
> 前置：只需第 1 章。不要求有板子——配置阶段的输出本机就能看。
> 说明：本章命令与输出按 CMake 3.16+ 标准格式整理，本机复跑路径附在每一步后面。
> **示例工程：** [`demo/`](./demo/)——与第 1 章同款三文件工程，配两份 toolchain file。

## 章节导航

| 节 | 标题 | 一句话 |
|----|------|--------|
| [2.1](./2.1-编译机和运行机不是同一台.md) | 编译机和运行机不是同一台 | 交叉编译的问题起点；CMake 把工具链信息抽出工程描述 |
| [2.2](./2.2-交叉编译只比本机编译多三件事.md) | 交叉编译只比本机编译多三件事 | 给谁跑 / 编译器换谁 / 去哪找库；三元组 |
| [2.3](./2.3-一份toolchain-file逐行解剖.md) | 一份 toolchain file 逐行解剖 | SYSTEM_NAME 总开关 / TRY_COMPILE_TARGET_TYPE 灵魂一行 / _INIT flags |
| [2.4](./2.4-跑一次工程描述一个字不改.md) | 跑一次：工程描述一个字不改 | 只多传 `-DCMAKE_TOOLCHAIN_FILE`；输出逐行读 + `file` 验证 |
| [2.5](./2.5-交叉编译的找库边界.md) | 交叉编译的找库边界 | find_* 三组开关：PROGRAM=NEVER / LIBRARY、INCLUDE=ONLY + CMAKE_SYSROOT |
| [2.6](./2.6-本章验收清单.md) | 本章验收清单 | 五条自查 |
| [2.7](./2.7-与后续衔接.md) | 与后续衔接 | 第 3 章多目录 / 08 模块 sysroot / 缓存对比小练习 |

> 快速上手（30 秒版）：`cmake -B build-m3 -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake && cmake --build build-m3`

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
