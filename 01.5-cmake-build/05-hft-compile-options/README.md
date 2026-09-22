# 第 5 章 · HFT 编译选项工程化

> **本节讲什么：** `-O3`、`-march=native`、LTO、ASan——这些 flag 在 Makefile 里
> 都是随手一写。HFT 工程的真正问题是：**哪套 flag 给哪个环境用，怎么保证所有人、
> 所有机器、所有 CI 跑出来的二进制是一致的**。这章把编译选项从"命令行参数"
> 升级成"工程资产"：构建类型、per-target 优化、sanitizer 开关、CMakePresets。
>
> 前置：第 1、3 章。说明：命令与输出按 CMake 3.21+ / clang/gcc 标准格式整理。

---

## 5.1 问题的起点：flag 散落在三个地方

一个真实的 HFT 小团队常见事故现场：

| 事故 | 根因 |
|---|---|
| 压测机上延迟漂亮，上线后毛刺一堆 | 压测用 `-O3 -march=native` 手工编的，上线包是 CI 默认参数编的——**比较的不是同一个二进制** |
| 同事编不出同样的 .so | flag 写在"老大机器的 .bashrc 别名"里，新人环境永远差一点 |
| Debug 版上线 | 构建脚本忘了传 `-O2`，`-O0` 的二进制跑了一周才发现延迟翻倍 |

Makefile 时代这些靠"纪律"防；CMake 提供的是**机制**：把每套环境的 flag 组合
固化成文件、纳入 git 评审，让"压测和上线不同二进制"在结构上不可能发生。

## 5.2 CMAKE_BUILD_TYPE：四档预设，先选档再微调

CMake 内置四档构建类型，每档是一组默认 flag（单配置生成器：Makefile/Ninja）：

| 档位 | 默认 flag（gcc/clang） | HFT 场景的用途 |
|---|---|---|
| `Debug` | `-g -O0` | 调 bug：零优化，变量不被优化掉，gdb 里所见即所得 |
| `Release` | `-O3 -DNDEBUG` | 上线包：全优化 + 关掉 assert |
| `RelWithDebInfo` | `-O2 -g -DNDEBUG` | **压测/性能分析首选**：优化接近上线，还留着符号给 perf |
| `MinSizeRel` | `-Os -DNDEBUG` | 嵌入式固件体积敏感时用 |

指定方式：`cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo`。
**不给默认值是新手最常见的坑**——CMakeLists 里补一行兜底（demo/ 里有）：

```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
```

注意 `RelWithDebInfo` 默认 `-O2`，HFT 热路径想要 `-O3` 怎么办？下一节：
**按档位追加自己的 flag**，而不是改全局变量。

## 5.3 per-target、per-config：优化挂在哪里

第 3 章的原则在这里收利息：**优化是 target 的属性，不是工程的泼水**。
行情解析库要 `-O3 -march=native`，但日志工具、测试程序不需要——
全工程 `-march=native` 意味着连调试工具都被绑死在特定 CPU 上。

按档位挂 flag 用**生成器表达式**（`$<CONFIG:...>`，配置阶段不求值、生成阶段才展开）：

```cmake
target_compile_options(latency_demo PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native>
    $<$<CONFIG:Debug>:-O0 -g3>)
```

**`-march=native` 的部署前提（HFT 必须知道的一条）**：它让编译器按**编译机**的
CPU 特性生成指令（AVX-512 等）。编译机有 AVX-512、线上机没有 → 线上一执行到那条
指令就是 `SIGILL`（非法指令，进程直接死）。三个安全的姿势：

1. 编译机 = 线上机同型号 CPU（HFT 常见：专用构建机对齐线上硬件）；
2. 不用 `native`，显式写目标微架构：`-march=icelake-server`（可评审、可复现）；
3. 交叉/异构部署走第 2 章 toolchain file，把 `-march` 写在那里。

## 5.4 LTO：让内联跨过 .c 文件的边界

普通编译以 .c 为单位：`ma.c` 里的函数没法内联进 `risk.c`——热路径上隔着
编译单元的函数调用，就是延迟。LTO（Link Time Optimization）把优化推迟到链接期：
**链接器拿到所有编译单元的中间表示，跨文件内联、跨文件消死代码**。

Makefile 里要手工给编译和链接都加 `-flto`，CMake 抽象成一个属性：

```cmake
set_target_properties(latency_demo PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)   # 只给 Release 档开
```

两个心理预期：①链接明显变慢（正常，优化在链接期干活）；
②调试体验变差（跨文件内联后，gdb 里"函数"边界模糊）——所以只给 Release 开，
Debug 保持直来直去。这也是 5.2 说"按档配置"的又一个实例。

## 5.5 Sanitizers：开发期的 bug 收割机

`-O3` 的二极管另一面：**正确性工具**。03.6 模块讲过 ASan/UBSan 的定位，
这里只解决"在 CMake 里怎么管"——编译和链接**成对**挂，用 `option()` 做成开关：

```cmake
option(ENABLE_ASAN "AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(demo PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(demo PRIVATE -fsanitize=address)   # 漏了链接这半：undefined reference to __asan_*
endif()
```

打开：`cmake -B build/dev -DENABLE_ASAN=ON`。demo/main.c 里留了一处注释掉的越界，
取消注释后用 ASan 构建跑一遍，就是一份标准的 buffer-overflow 报告——
**开发机上每次全量测试都带 sanitizer 跑，是 HFT 团队成本最低的质量投资**。
（线上不开：ASan 有 2x 量级减速，那是 03.6 讲的"开发期工具"。）

## 5.6 CMakePresets.json：把"哪套环境用哪套 flag"固化进 git

到这一步，参数已经很多：构建类型、sanitizer 开关、build 目录、生成器……
`CMakePresets.json` 把它们命名成**预设**，进 git、进评审：

```json
{ "name": "dev",
  "binaryDir": "${sourceDir}/build/dev",
  "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug",
                      "ENABLE_ASAN": "ON", "ENABLE_UBSAN": "ON" } }
```

使用：`cmake --preset dev && cmake --build build/dev`。
demo/ 里给了三个预设：`dev`（调试+双 sanitizer）/ `bench`（压测档）/ `prod`（上线档）。

5.1 的三起事故到此全部有机制兜底：**参数在 git 里、有名字、所有人用同一套名字**。
"你压测用的哪个 preset？"成为团队的标准问法。

## 5.7 本章验收清单

- [ ] 四档构建类型的默认 flag 和 HFT 用途能对号入座，知道不给默认值的坑
- [ ] 会用生成器表达式按 CONFIG 挂 per-target flag
- [ ] 说清 `-march=native` 的部署前提和三个安全姿势
- [ ] LTO 解决什么问题、为什么只给 Release 开
- [ ] sanitizer 为什么编译链接要成对，`-DENABLE_ASAN=ON` 全流程走一遍
- [ ] 写出自己项目的三个 preset（dev / bench / prod）

## 5.8 与后续衔接

- **03.6-userspace-debugging**：sanitizer 报出来的东西怎么读、core dump 怎么配——那边是工具课
- **06.6-systems-performance**：`-O3` 之后还想快，就进入 perf / 火焰图的世界
- **20-compilers-llvm**：`-march`、LTO 在编译器内部是怎么实现的
- 小练习：`bench` preset 编出的二进制和 `dev` 的对比 `size` 和 `objdump -d | wc -l`，
  直观感受 LTO + O3 消掉了多少代码

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
