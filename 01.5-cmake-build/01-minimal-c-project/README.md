# 第 1 章 · 最小工程：CMake 到底在干什么

> **本章讲什么：** 你还不知道 CMake 是什么——很好，这章从你**已经会的东西**（手敲 gcc）出发，
> 三级台阶走到 CMake，然后把 `build/` 目录拆开看它到底替你干了什么。
> 全部命令在 macOS 26.6.2（CMake 4.4.3，cdev 环境实测；gcc 视角书写，
> clang 兼容 gcc 用法，本章场景下逐行等价）。
> **示例工程：** [`demo/`](./demo/)——三文件小工程（移动平均 + 风控夹取），可亲手复跑。

## 章节导航

| 节 | 标题 | 一句话 |
|----|------|--------|
| [1.1](./1.1-编译命令什么时候失控.md) | 编译命令什么时候失控 | 从手敲 gcc 出发，看它怎么在增量性和环境两个维度失控 |
| [1.2](./1.2-构建系统只管两件事.md) | 构建系统只管两件事 | 依赖关系 + 动作命令；CMake 在 Makefile 的上一层 |
| [1.3](./1.3-三级台阶.md) | 三级台阶 | 同一个工程：手动 gcc → Makefile → CMakeLists 逐行注解 |
| [1.4](./1.4-两条命令每一步发生了什么.md) | 两条命令拆解 | `cmake -B build` / `cmake --build build` 的真实输出逐行读 |
| [1.5](./1.5-拆开build目录.md) | 拆开 build/ | CMakeCache / flags.make / link.txt / **.o.d（依赖自动化的答案）** |
| [1.6](./1.6-现代CMake三原则.md) | 现代 CMake 三原则 | 一切皆 target / 属性挂 target / 禁止全局操作 |

> 快速上手（30 秒版）：`cmake -B build && cmake --build build && ./build/demo`

## 代码自测

<details><summary>Q1：cmake 和 make 分别是什么？一条命令各在哪个阶段起作用？</summary>

CMake 是**构建系统生成器**：读 CMakeLists.txt（工程描述），探测本机环境，
生成本机构建系统（macOS/Linux 默认 Makefile，也可生成 Ninja/VS 工程）——`cmake -B build` 这一步。
make 是**构建系统**：按生成的 Makefile 里的依赖规则和命令真正编译——`cmake --build build`
（= 进 build/ 执行 make）这一步。一句话：**cmake 描述工程，make 执行编译。**
</details>

<details><summary>Q2：改了 math_utils.h 之后重新构建，main.c 会重编吗？谁保证的？</summary>

会。保证者是 `main.c.o.d`（见 [1.5](./1.5-拆开build目录.md)）：编译 main.c 时编译器被要求
（CMake 自动加 `-MMD`）顺手生成"我 include 了哪些头文件"的清单，make 每次构建读它来决定重编谁。
这就是 Makefile 里 `main.o: main.c math_utils.h` 那行手写依赖的自动化版本——而且永远不会写漏。
</details>

<details><summary>Q3：为什么 build 目录要 out-of-source（在源码树外面）？</summary>

三个理由：①源码树干净，`.gitignore` 一个 `build/` 全解决；②同源码可配多个不同
build 目录（debug/release/交叉编译并存）；③删除 build/ 即完全重来，配置坏了不用找脏文件。
CMake 会把缓存（CMakeCache.txt）锁在配置时的 build 目录里，混进源码树反而会出事。
</details>

<details><summary>Q4：PRIVATE / PUBLIC / INTERFACE 的区别？</summary>

属性的**传播范围**：`PRIVATE` 只作用于本 target 的编译；
`PUBLIC` 本 target 编译要用 + 依赖我的 target 也要用（典型：实现文件和头文件都
include 的库）；`INTERFACE` 本 target 不用、只有依赖我的用（典型：header-only 库）。
类比 05 书 CH4 的接口语义：这是 CMake 版的"谁需要看见这个声明"。
</details>

<details><summary>Q5：你的工程 100 个 .c 文件，add_executable 一行要写 100 个文件名吗？</summary>

不应该写死。现代做法：按目录拆 `add_subdirectory`（每个子目录自己的 CMakeLists 建
target，第 3 章讲）。`file(GLOB)` 官方不推荐——新增/删除文件不会触发重新配置。
"显式列文件"在这里不是笨，是让构建系统知道边界。
</details>
