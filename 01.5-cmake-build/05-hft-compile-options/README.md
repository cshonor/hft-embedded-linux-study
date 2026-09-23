# 第 5 章 · HFT 编译选项工程化

> **本章讲什么：** `-O3`、`-march=native`、LTO、ASan——这些 flag 在 Makefile 里
> 都是随手一写。HFT 工程的真正问题是：**哪套 flag 给哪个环境用，怎么保证所有人、
> 所有机器、所有 CI 跑出来的二进制是一致的**。这章把编译选项从"命令行参数"
> 升级成"工程资产"：构建类型、per-target 优化、sanitizer 开关、CMakePresets。
>
> 前置：第 1、3 章。说明：命令与输出按 CMake 3.21+ / clang/gcc 标准格式整理。
> **示例工程：** [`demo/`](./demo/)——含三个 preset（dev / bench / prod）与 sanitizer 开关。

## 章节导航

| 节 | 标题 | 一句话 |
|----|------|--------|
| [5.1](./5.1-flag散落在三个地方.md) | flag 散落在三个地方 | 三起真实事故：压测与上线比的不是同一个二进制 |
| [5.2](./5.2-CMAKE_BUILD_TYPE四档预设.md) | CMAKE_BUILD_TYPE 四档预设 | Debug / Release / RelWithDebInfo / MinSizeRel；不给默认值的坑 |
| [5.3](./5.3-优化挂在哪里.md) | 优化挂在哪里 | 生成器表达式按档挂 per-target flag；`-march=native` 的 SIGILL 前提 |
| [5.4](./5.4-LTO让内联跨过c文件边界.md) | LTO：让内联跨过 .c 边界 | 跨文件内联 + 消死代码；只给 Release 开 |
| [5.5](./5.5-Sanitizers开发期的bug收割机.md) | Sanitizers | 编译链接成对挂，`option()` 做成开关；开发期最便宜的质量投资 |
| [5.6](./5.6-CMakePresets固化进git.md) | CMakePresets.json | "环境名 → 参数组合"固化进 git：dev / bench / prod |
| [5.7](./5.7-本章验收清单.md) | 本章验收清单 | 六条自查（含写出自己的三个 preset） |
| [5.8](./5.8-与后续衔接.md) | 与后续衔接 | 03.6 调试工具 / 06.6 性能分析 / 20 编译器实现 |

> 快速上手（30 秒版）：`cmake --preset dev && cmake --build build/dev`

## 代码自测

<details><summary>Q1：Release 和 RelWithDebInfo 的区别？HFT 压测为什么用后者？</summary>

默认 flag：Release = -O3 -DNDEBUG；RelWithDebInfo = -O2 -g -DNDEBUG。
差别在调试信息（-g）和默认优化档。压测用 RelWithDebInfo 两个理由：
①-g 保留符号，perf 采样、火焰图才能把地址翻译回函数名——没符号的性能数据
几乎不可读；②-O2 与上线的 -O3 有差距，可用 per-config flag 把热路径 target
补到 -O3（demo/ 的做法），兼顾"接近上线性能"与"可分析"。
纯 Debug 压测没意义：-O0 的延迟分布和优化后完全是两个程序。
</details>

<details><summary>Q2：-march=native 为什么会在线上机 SIGILL？怎么避免？</summary>

native 让编译器按编译机 CPU 的全部指令集特性生成代码（如 AVX-512）。
线上机 CPU 较老不支持这些指令时，程序执行到对应指令触发非法指令异常
（SIGILL），进程直接被杀——且通常在运行到特定代码路径才崩，极具隐蔽性。
避免姿势：①构建机与线上机 CPU 同型号（HFT 常用）；②显式指定微架构
（-march=icelake-server 等，写进 CMake/toolchain file 可评审）；
③多型号部署用 -march=x86-64-v2/v3 这种分级基线。核心原则：
指令集目标必须显式、可复现，不能"看编译机心情"。
</details>

<details><summary>Q3：LTO 为什么既能提速又能减体积？代价是什么？</summary>

普通编译以 .c 为边界，跨文件的函数调用无法内联。LTO 让编译器输出中间表示
（GIMPLE/LLVM IR），链接期统一优化：跨文件内联（热路径省去调用开销）、
跨文件死代码消除（只被废弃路径引用的函数整体移除）。提速来自前者，
减体积来自后者。代价：①链接时间显著变长（优化工作挪到了链接期）；
②调试体验下降（内联抹掉函数边界，gdb 断点/回溯变"糊"）；
③对工具链版本敏感（混用不同版本编译器的 .o 可能失败）。
所以 CMake 里按档开关：INTERPROCEDURAL_OPTIMIZATION_RELEASE ON，Debug 不开。
</details>

<details><summary>Q4：ASan 的 -fsanitize=address 为什么编译和链接都要加？</summary>

ASan 由两部分组成：编译期插桩（编译器在每次内存访问前后插入"这块内存
能不能碰"的检查代码）和运行时库（libasan：维护影子内存、报错、打印堆栈）。
只加编译选项：插桩代码里引用的 __asan_* 符号在链接时找不到 →
undefined reference。只加链接选项：没有插桩，等于白链接。
target_compile_options + target_link_options 成对挂是唯一正确姿势；
用 CMake 的 if(ENABLE_ASAN) 包起来，防止"加了半边"的半成品状态进 git。
</details>

<details><summary>Q5：CMakePresets.json 解决了什么 Makefile 解决不了的问题？</summary>

Makefile 时代的 flag 组合活在三个不可靠的地方：文档/wiki（会过时）、
老员工的记忆（会离职）、CI 脚本（和本地不一致）。CMakePresets.json 把
"环境名 → 完整参数组合"固化成 git 管理的文件：①可评审——改 flag 走 PR，
有 diff 有讨论；②本地与 CI 同源——CI 也跑 cmake --preset prod，
"本地能编 CI 挂"大幅减少；③新人零成本——cmake --preset dev 一条命令
拿到和全组一致的环境。本质：把构建知识从"人"转移到"资产"。
</details>
