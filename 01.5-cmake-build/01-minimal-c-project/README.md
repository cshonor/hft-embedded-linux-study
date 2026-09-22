# 第 1 章 · 最小工程：CMake 到底在干什么

> **本节讲什么：** 你还不知道 CMake 是什么——很好，这章从你**已经会的东西**（手敲 gcc）出发，
> 三级台阶走到 CMake，然后把 `build/` 目录拆开看它到底替你干了什么。
> 全部命令在 macOS 26.6.2（clang 23.1.0 / CMake 4.x）实测，输出原样贴出。

---

## 1.1 问题的起点：编译命令什么时候失控

一个 `main.c`，编译是一条命令：

```bash
clang main.c -o demo
```

程序长大：拆出 `math_utils.c`，加了 `-Wall -std=c11`，还得管头文件依赖：

```bash
clang -Wall -Wextra -std=c11 main.c math_utils.c -o demo
```

实测（本仓库 `demo/` 工程的真实输出）：

```text
$ clang -Wall -Wextra -std=c11 main.c math_utils.c -o demo_manual && ./demo_manual
n=5  MA=100.4800  reported=100.4800
```

还能忍。但真正的失控在**增量性**和**环境**两个维度上：

| 失控点 | 具体表现 |
|---|---|
| 改一个文件要重编全量 | 上面那条命令永远编译所有源文件，工程 100 个文件时每次改一行都全量重编 |
| 头文件依赖没人管 | `math_utils.h` 改了，你忘了重编 `main.c`，链接出来的还是旧逻辑——**不报错** |
| 换平台命令就作废 | Linux 加 `-ldl -lpthread`，macOS 不要；Windows 是另一套编译器、另一套 flag |
| 找库是玄学 | 库装在哪？头文件在哪？每个装库的方式都不一样 |

## 1.2 第一性原理：构建系统只管两件事

剥掉所有工具的外壳，"构建"这件事只包含两种信息：

1. **依赖关系** —— 谁依赖谁：`demo` ← `main.c.o` + `math_utils.c.o`；
   `main.c.o` ← `main.c` + `math_utils.h`
2. **动作命令** —— 每个依赖节点上执行什么：`clang -c main.c -o main.c.o`

**Makefile 就是把这两件事写成规则**。你仓库里 147 个 demo Makefile 都是这么干的。
那为什么还要 CMake？因为 Makefile 把这两件事**写死在了"本机"视角**：

- 依赖和命令你自己写 → 写错依赖就出上面说的"不报错的旧逻辑"
- 它生成的构建过程绑死当前平台 → 换平台重写

**CMake 的位置在 Makefile 的上一层**：

```text
你写的          CMake 替你生成的         真正干活的
CMakeLists.txt → Makefile / Ninja / VS 工程 → clang / gcc / MSVC
（描述工程）      （本机构建系统，自动生成）    （编译命令，自动拼装）
```

你只写**一份**平台无关的"工程描述"；CMake 探测本机环境（编译器在哪、库在哪、
什么平台），替你**生成**对应的 Makefile。换平台 = 换个目录重新生成一遍，你一个字不用改。

> 类比：Makefile 是"菜谱"（你写清每一步）；CMakeLists 是"点菜"（你说要什么菜，
> 后厨——本机工具链——怎么配合它替你安排）。

## 1.3 三级台阶：同一个工程的三种构建法

`demo/` 是同一个三文件工程（`main.c` + `math_utils.c` + `math_utils.h`，
算一个移动平均再夹到风控区间——刻意带点 HFT 味）：

**台阶一：手动编译**（见 1.1，一条长命令，全量、无依赖管理）

**台阶二：Makefile**——你要手写依赖规则：

```makefile
demo: main.o math_utils.o
	clang main.o math_utils.o -o demo
main.o: main.c math_utils.h      # ← 依赖要自己写对
	clang -Wall -std=c11 -c main.c
math_utils.o: math_utils.c math_utils.h
	clang -Wall -std=c11 -c math_utils.c
```

增量编译解决了，但规则是**手写的、平台绑定的**。`STM32-/labs/00` 的 Makefile
你见过它的完全体：`--target=armv7m-none-eabi` 一大串交叉编译 flag 全写死在里面。

**台阶三：CMake**——你只描述"目标是什么"：

```cmake
cmake_minimum_required(VERSION 3.16)   # ① 语法基准线
project(demo C)                        # ② 工程名 + 语言
add_executable(demo main.c math_utils.c)  # ③ 一切皆 target
target_compile_options(demo PRIVATE -Wall -Wextra)  # 属性挂在 target 上
set_target_properties(demo PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
```

对比一下心智负担的转移：

| | Makefile | CMakeLists |
|---|---|---|
| 依赖关系 | **自己写**（写错不报错） | 编译器扫描自动生成（`.o.d` 文件，见 1.5） |
| 编译命令 | 自己拼 | CMake 按探测到的编译器自动拼 |
| 换平台 | 重写 | 重新 `-B` 生成一遍 |
| 找库 | 自己写 `-I`/`-L`/`-l` | `find_package` / `target_link_libraries` |

## 1.4 两条命令，每一步发生了什么

```bash
cmake -B build        # 配置阶段
cmake --build build   # 构建阶段
```

**配置阶段**（`cmake -B build`）真实输出：

```text
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /Users/a0000/micromamba/envs/cdev/bin/clang - skipped
-- Configuring done (1.1s)
-- Generating done (0.0s)
-- Build files have been written to: .../demo/build
```

读输出就是读 CMake 在干什么：**探测编译器**（找到 cdev 环境的 clang）→
**执行你的 CMakeLists**（`add_executable` 等只是登记 target）→
**生成**本机构建系统（macOS 上默认生成 Makefile）。

**构建阶段**（`cmake --build build`）真实输出：

```text
[ 33%] Building C object CMakeFiles/demo.dir/main.c.o
[ 66%] Building C object CMakeFiles/demo.dir/math_utils.c.o
[100%] Linking C executable demo
[100%] Built target demo
```

注意 `cmake --build` 是个**包装**：它调用的是 build/ 里刚生成的构建系统
（这里是 make）。好处：不管底下生成的是 Makefile 还是 Ninja 还是 VS 工程，
这条命令永远不变——这就是"跨平台"的落点。

## 1.5 拆开 build/：CMake 生成的东西一览

```text
build/
├── CMakeCache.txt          ← 探测结果缓存（编译器路径、平台、选项），配置阶段写入
├── Makefile                ← 生成的本机构建系统（构建阶段的真正入口）
├── cmake_install.cmake     ← 安装规则（install() 命令的产物）
└── CMakeFiles/demo.dir/
    ├── flags.make          ← 每个文件的编译 flag（你的 -Wall 在这，可核对）
    ├── link.txt            ← 完整链接命令行（链接出错时看这个）
    ├── main.c.o            ← 目标文件
    ├── main.c.o.d          ← ★ 依赖清单：main.c 依赖哪些头文件，编译器自动生成
    └── math_utils.c.o(.d)
```

**`main.c.o.d` 就是 1.2 说的"依赖没人管"的答案**：CMake 让编译器（`-MMD`）
在编译时顺手生成"我到底 include 了谁"的清单，下次构建时喂给 make——
`math_utils.h` 一改，`main.c.o` 自动重编。你在 Makefile 里手写的那行
`main.o: main.c math_utils.h`，CMake 替你生成且**永远不会写错**。

两个实测核对（学构建系统的好习惯：怀疑生成物，就去打开它）：

```bash
grep '\-Wall' build/CMakeFiles/demo.dir/flags.make   # 你的 flag 真的生效了
cat build/CMakeFiles/demo.dir/link.txt              # 看完整的链接命令
```

> **为什么源码树是干净的？** 这叫 **out-of-source build**：所有生成物都在 `build/`，
> 删掉 `build/` = 完全重来。源码目录里只有你写的东西 + `CMakeLists.txt`。
> 把 `build/` 写进 `.gitignore`，这是 CMake 工程的标准姿势。

## 1.6 现代 CMake 三原则（从第一个工程就养成）

1. **一切皆 target**。`add_executable` / `add_library` 创建 target，
   之后的编译选项、include 路径、链接库全部**挂在 target 上**。
2. **属性用 `target_*` 命令挂**。`target_compile_options` / `target_include_directories` /
   `target_link_libraries`，并写明传播范围：`PRIVATE`（只我用）/ `PUBLIC`（我和依赖我的都用）/
   `INTERFACE`（只有依赖我的用）。
3. **禁止全局操作**。`include_directories`、`add_compile_options`、
   手写 `${CMAKE_SOURCE_DIR}/include` 都是老式写法——单文件工程看不出坏处，
   多目录工程里全局 include 路径会互相污染，最终谁也说不清哪个 target 依赖哪个头。

为什么从第 1 章就讲这个：**老式写法的教程会把你的心智模型带歪**，
先记住"对的样子"，见到不对的才认得出来。

## 1.7 与后续衔接

- **第 2 章** 把交叉编译 flag 挪进 toolchain file——`STM32-/labs/00` 的 Makefile
  里那串 `--target=armv7m-none-eabi -mcpu=cortex-m3 -mthumb` 在 CMake 里的正确归宿
- **ch6 §6.2.4**（概念入口）→ 本章（动手）→ 第 4 章（解剖 Zephyr 的 `west build`，
  它就是本章这套机制的官方大规模应用）
- 下一章之前的小练习：把 `demo/CMakeLists.txt` 里的 `target_compile_options` 改成
  老式 `add_compile_options(-Wall -Wextra)`，重跑，再到 `flags.make` 里找差别——
  单 target 时结果一样，体会一下"为什么多 target 时会出事"

## 代码自测

<details><summary>Q1：cmake 和 make 分别是什么？一条命令各在哪个阶段起作用？</summary>

CMake 是**构建系统生成器**：读 CMakeLists.txt（工程描述），探测本机环境，
生成本机构建系统（macOS/Linux 默认 Makefile，也可生成 Ninja/VS 工程）——`cmake -B build` 这一步。
make 是**构建系统**：按生成的 Makefile 里的依赖规则和命令真正编译——`cmake --build build`
（= 进 build/ 执行 make）这一步。一句话：**cmake 描述工程，make 执行编译。**
</details>

<details><summary>Q2：改了 math_utils.h 之后重新构建，main.c 会重编吗？谁保证的？</summary>

会。保证者是 `main.c.o.d`：编译 main.c 时编译器被要求（CMake 自动加 `-MMD`）顺手生成
"我 include 了哪些头文件"的清单，make 每次构建读它来决定重编谁。这就是 Makefile 里
`main.o: main.c math_utils.h` 那行手写依赖的自动化版本——而且永远不会写漏。
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
类比 CH4 的接口语义：这是 CMake 版的"谁需要看见这个声明"。
</details>

<details><summary>Q5：你的工程 100 个 .c 文件，add_executable 一行要写 100 个文件名吗？</summary>

不应该写死。现代做法：要么按目录拆 `add_subdirectory`（每个子目录自己的
CMakeLists 建 target，第 3 章讲），要么用 `file(GLOB)` 收集源文件——
但 `file(GLOB)` 官方不推荐（新增/删除文件不会触发重新配置），G 组目录式组织 +
显式列文件才是正解。"显式"在这里不是笨，是让构建系统知道边界。
</details>
