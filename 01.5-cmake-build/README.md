# 01.5 · CMake 构建系统：C/C++ 工程的组织术

> **本模块回答一个问题：** 一个 C/C++ 工程，怎么让构建系统替我管编译、链接、交叉编译——而不是每个工程手搓一遍 Makefile。
> **定位：** 工具型模块（同 20-compilers-llvm 的"工具"属性），服务三条线：`projects/`（P10 HFT 原型已在用 CMake）、`STM32-` 仓库（裸机交叉编译 + Zephyr 必经之路）、本仓库多文件 demo 的进阶形态。
> **组织方式：** 任务节点（同 08 的骨架），书降级为工具书，见 [`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)。
> **纪律：** 每个节点先跑通再落笔。好消息：五个节点全部 Mac 上可跑通，**无板依赖，不用等货**。

---

## 任务节点

| # | 节点 | 交付物 | 绑定的真实工程 |
|---|------|--------|----------------|
| 01 | [minimal-c-project](./01-minimal-c-project/) | 把一个 Makefile 工程迁成 CMakeLists，说清 target / 属性 / 现代 CMake 原则 | `STM32-/labs/00-toolchain-clang`（已有，最小工程） |
| 02 | toolchain-file | 交叉编译工具链文件：`--target=armv7m-none-eabi` 这套从 Makefile 挪进 toolchain file | 同上 |
| 03 | libs-and-multi-dir | 静态库 + 多目录组织（`add_subdirectory` / target 链接） | STM32- labs/04 uart 阶段的真实需求 |
| 04 | zephyr-build-anatomy | 解剖 `west build` 背后的 CMake：Kconfig ↔ CMake 联动、devicetree 生成物 | Zephyr `qemu_cortex_m3`（无板跑通） |
| 05 | hft-build-flags | `-O3` / `-march` / LTO / sanitizers 的工程化管理 | `projects/P10-hft-prototype` |

---

## 为什么编号插队在 01 之后（而不是 21）

编号 = 学习动线。CMake 是"写 C 工程的工具"，学完语言基础就该有，不该排在 20 个领域模块之后。
本仓库已有 `03.5` / `05.5` / `06.5` / `11.5` 等 x.5 插入先例，沿用之；**永远不做中间重编号**（上次 CHn.x 重排的回旋镖教训：批量规则会误伤跨章引用）。

## 与既有内容的分工（避免重复建设）

- **05 书 ch6 §6.2.4「CMake 基础」**：58 行入门薄笔记，回答"CMake 是什么、基本流程"。
  本模块是它的**展开与实操版**：迁真实工程、toolchain file、Zephyr 解剖——先读它建立概念，再来这里动手。
- **05 书 ch4 §4.2「Linux 内核编译」+ ch4 各节的 KBuild/Makefile 内容**：那是 **Kbuild**——内核
  自有的 make 体系（Kconfig/obj-y），和 CMake 是两个世界，内核线不迁 CMake。此处只在一处交汇：
  节点 04 解剖 `west build` 时会对照"Kconfig↔CMake 联动"与"Kconfig↔Kbuild 联动"的异同。
- **05 书 ch5 §5.19「模块的编译和链接」**：多文件工程为什么需要构建系统，是本模块的问题来源。

## 与 20-compilers-llvm 的分工

- **20** 讲编译器内部：前端 / IR / 后端，回答"编译器怎么工作"
- **本模块**讲构建系统：target / 依赖 / 工具链文件，回答"怎么指挥编译器干活"
- 一个是原理，一个是工程组织，不重叠

## 现状盘点（2026-09-22，为什么这么定）

- hft 仓库 147 个 Makefile（单目录小 demo，**Make 是对的工具，不动**）、2 个 CMakeLists（`projects/P10-hft-prototype/part-a-demo` + `01-c-language/.../demo03_cmake`，边界正确）
- 仓库级构建是 `build_all.py`（笔记 HTML 生成器），与 C 编译无关，CMake 无位置
- 规则：**`projects/` 下真项目用 CMake；各章 demo 用 Make；笔记构建归 build_all.py**

## 甄别原则（看任何 CMake 教材都带上这条）

一切皆 target、属性挂在 target 上（现代 CMake）。见到 `include_directories`、
手写 `${CMAKE_SOURCE_DIR}`、全局 flag 一把梭的写法，一律按老式写法处理——
语法可能没错，但组织方式过时。两本书里《Professional CMake》modern 立场最正。
