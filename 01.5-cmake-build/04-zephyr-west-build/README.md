# 第 4 章 · 解剖 `west build`：Zephyr 里的 CMake

> **本章讲什么：** 前三章的 CMake 是自己写 CMakeLists。这章换个视角：
> 看一个**工业级项目**（Zephyr RTOS）怎么把同一套机制用到几百个模块的规模。
> 一句 `west build` 背后：west → cmake 的调用链、Kconfig 怎么决定哪些源文件进构建、
> 以及它和 Linux 内核 Kbuild 那对"表兄弟"的逐项对照。
> 这章是**读代码课**——demo/ 是一个最小 Zephyr 应用的标本，目标是"看得懂"，不要求有板子。
>
> 前置：第 1、3 章（target / add_subdirectory / target_sources）。
> 说明：输出示例取自 Zephyr 3.x 构建日志的标准格式。
> **示例工程：** [`demo/`](./demo/)——最小 Zephyr 应用标本（CMakeLists + prj.conf）。

## 章节导航

| 节 | 标题 | 一句话 |
|----|------|--------|
| [4.1](./4.1-一句west-build谁干的活.md) | 一句 `west build`，谁干的活？ | west 拼参数、cmake 施工、ninja 执行——`-v` 第一行就露底 |
| [4.2](./4.2-应用的CMakeLists为什么长得不一样.md) | 应用的 CMakeLists 为什么长得不一样 | find_package 前置到 project() 之前；app target 是 Zephyr 建好的 |
| [4.3](./4.3-配置怎么变成代码.md) | Kconfig ↔ CMake：配置怎么变成代码 | `prj.conf` → `.config` → `autoconf.h` → 构建裁剪的四站链路 |
| [4.4](./4.4-与Linux内核Kbuild的对照.md) | 与 Linux 内核 Kbuild 的对照 | 同一条 Kconfig 血脉，"配置→构建动作"管道从 Makefile 换成 CMake |
| [4.5](./4.5-拆开Zephyr的build目录.md) | 拆开 Zephyr 的 build/ | 三层排查法：`.config` / `autoconf.h` / `build.ninja` 各管一段 |
| [4.6](./4.6-本章验收清单.md) | 本章验收清单 | 五条自查 |
| [4.7](./4.7-与后续衔接.md) | 与后续衔接 | 09 设备树 / 08 Buildroot / menuconfig 小练习 |

> 快速上手（30 秒版）：`west build -b <board> <app>`，然后进 `build/` 用第 1 章的方法拆。

## 代码自测

<details><summary>Q1：west 和 cmake 是什么关系？west build 之后还能用 cmake --build 吗？</summary>

west 是 Zephyr 生态的元工具：拼好 cmake 的全部参数（源码目录、build 目录、
BOARD、toolchain file、生成器）再调用 cmake，另外负责多仓库管理。
构建目录生成后就是普通 CMake build 目录——`cmake --build build` 照样可用，
增量构建时 west 内部调的也是它。类比：west 是"装修项目经理"，
真正干活的水电工还是 cmake + ninja。
</details>

<details><summary>Q2：为什么 Zephyr 应用用 target_sources(app ...) 而不是 add_executable？</summary>

因为固件这个"可执行文件"不只是应用的代码——它要链接 Zephyr 内核、驱动、
C 库、板级启动代码，链接脚本也由 Zephyr 按板子生成。Zephyr 预先创建好
app target 并把整套链接逻辑挂好，应用只声明"我的源文件有哪些"。
这是"一切皆 target"的规模化用法：框架提供骨架 target，用户往骨架上挂肉。
自己 add_executable 等于把内核构建逻辑全推给应用自己写。
</details>

<details><summary>Q3：prj.conf 里 CONFIG_GPIO=y 是怎么一步步影响到固件内容的？</summary>

四站：①合并——prj.conf 与板级默认、命令行 -D 合并成 build/.config；
②翻译——Kconfig 工具生成 autoconf.h，CONFIG_GPIO=y 变 #define CONFIG_GPIO 1；
③裁剪代码——C 源码的 IS_ENABLED(CONFIG_GPIO) 为真，相关代码保留；
④裁剪构建——GPIO 子系统的 CMakeLists 按 CONFIG 条件把驱动源文件加入 target，
编译进固件。CONFIG_GPIO=n 时第③④站同时生效：代码和编译单元双双消失。
</details>

<details><summary>Q4：Zephyr 的 Kconfig 和 Linux 内核的 Kconfig 是什么关系？</summary>

同一条血脉：Zephyr 直接借用了内核的 Kconfig 语言与工具（菜单语法、依赖表达式、
autoconf.h 生成机制都一样）。差别在"配置翻译成构建动作"的管道：内核用 Kbuild
（Makefile 里 obj-$(CONFIG_X) += x.o），Zephyr 用 CMake（按 CONFIG 条件
target_sources）。另外内核支持运行时加载模块（.ko），Zephyr 没有运行时模块，
一切裁剪都在编译期完成——更契合单片机资源受限、无动态加载的场景。
</details>

<details><summary>Q5：menuconfig 里明明开了某个驱动，固件里却没有，怎么排查？</summary>

三层排查法（各管一段，逐层确认）：①配置层——查 build/.config 里该 CONFIG 是否
真的 =y（可能被依赖条件 silently 关掉：Kconfig 的 depends on 不满足时选项会被强制回退）；
②头文件层——查 autoconf.h 里有没有对应 #define；③构建层——在 build.ninja 里
grep 驱动源文件名，确认它有没有编译规则。三层逐一定位：①没有→Kconfig 依赖问题；
①有②没有→翻译环节异常（罕见）；①②有③没有→该子系统 CMakeLists 的条件判断问题。
</details>
