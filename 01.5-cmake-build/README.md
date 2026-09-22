# 01.5 · CMake 构建系统：从"手敲命令"到"描述工程"

> **这本书回答一个问题：** 我写的是 C，为什么还要学一个"写文件的工具"？
> 答案藏在一个所有人都会撞上的墙里：程序从 1 个文件长到 10 个文件的那天，
> 手敲的编译命令就管不过来了。Makefile 能救一次，但跨平台、找库、交叉编译这三件事，
> Makefile 每换一个环境都要重写一遍——**CMake 是"描述工程"的语言，
> 它替你在每个平台上生成对应的构建系统**。

> **怎么读：** 像书一样按章节顺序读。每一章 = 一个可以亲手跑通的练习，
> 全部在 Mac 上无板跑通，正文里的输出都是真实终端输出，不是示意。
> 前置要求：会用终端、知道 gcc/clang 是编译器、写过一个 hello.c。没了。

---

## 目录

| 章 | 标题 | 一句话 | 状态 |
|----|------|--------|------|
| [第 1 章](./01-minimal-c-project/) | **最小工程：CMake 到底在干什么** | 从手敲 gcc → Makefile → CMake 三级台阶，解剖 `build/` 目录里到底生成了什么 | ✅ 实测完成 |
| 第 2 章 | toolchain file：交叉编译怎么说 | `--target=armv7m-none-eabi` 这套从 Makefile 挪进 toolchain file，给 STM32- 仓库用 | ⬜ 跟随 STM32- 进度 |
| 第 3 章 | 库与多目录：工程长大之后 | `add_subdirectory` / `target_link_libraries`，static lib 组织术 | ⬜ 跟随 STM32- labs/04 |
| 第 4 章 | 解剖 `west build`：Zephyr 里的 CMake | Kconfig ↔ CMake 联动，与 Kbuild 的 Kconfig ↔ Kbuild 对照 | ⬜ 跟随 Zephyr 主线 |
| 第 5 章 | HFT 编译选项工程化 | `-O3` / `-march` / LTO / sanitizers 在 CMake 里的管理 | ⬜ 跟随 P10 |

## 快速上手（30 秒版）

```bash
cmake -B build        # 配置：读 CMakeLists.txt，在 build/ 里生成本机构建系统
cmake --build build   # 构建：调用刚生成的构建系统真正编译
./build/demo          # 跑
```

记住这两条命令，第 1 章会把每一步发生的事情拆开讲透。

## 与既有内容的边界

- **05 书 ch6 §6.2.4「CMake 基础」**：概念入口（58 行薄笔记）。先读它知道"CMake 是什么"，
  来这里动手。注意它的示例是老式写法，本模块按现代写法教。
- **05 书 ch4 的 KBuild 内容**：Kbuild 是内核自有的 make 体系，与 CMake 两个世界；
  唯一交汇在第 4 章解剖 `west build` 时的 Kconfig 对照。
- **20-compilers-llvm**：讲编译器内部（"编译器怎么工作"）；本模块讲构建系统（"怎么指挥编译器"）。

## 现状盘点（2026-09-22）

- hft 仓库 147 个 Makefile（单目录小 demo，**Make 是对的工具，不动**）、2 个 CMakeLists
- 规则：**`projects/` 下真项目用 CMake；各章 demo 用 Make；笔记构建归 build_all.py**
- 工具书（《CMake构建实战》《Professional CMake》）见 [`_refs/BOOK-MAP.md`](./_refs/BOOK-MAP.md)

## 甄别原则（看任何 CMake 教材都带上这条）

一切皆 target、属性挂在 target 上（现代 CMake）。见到 `include_directories`、
手写 `${CMAKE_SOURCE_DIR}`、全局 flag 一把梭的写法，一律按老式写法处理——
语法可能没错，但组织方式过时。两本书里《Professional CMake》modern 立场最正。
