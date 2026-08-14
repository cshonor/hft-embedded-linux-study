# 第 1 章 · 安装 LLVM · 本章定位

← [Part 01 索引](../README.md) · [Learn LLVM 17 总览](../../README.md) · 下一章 [02 编译器架构](../chapter02_compiler_arch/README.md)

> 对应 **《Learn LLVM 17》第 1 章** · 学习策略：**浏览**（见 [学习取舍](../../Learn-LLVM-17-学习取舍.md)）

---

## 本章讲什么

配置开发环境、**从源码编译 LLVM**、**CMake 自定义构建** — 为后续读 IR、改后端打基础。

| 与本书后续 | 关系 |
|------------|------|
| 只读 **rustc 导出 IR** / `opt` / `llc` | 装**二进制**或随 rustc 工具链即可，**不必**源码编译 |
| 改 TableGen / **自定义后端**（Part 04） | 源码编译 + 自定义 CMake **才有必要** |

---

## 阅读路线

| 节 | 主题 |
|----|------|
| [01 环境要求](./01-env-requirements.md) | 硬件 · 编译器 · 必备工具 |
| [02 五步构建](./02-build-five-steps.md) | clone → cmake → ninja → test → install |
| [03 自定义 CMake](./03-custom-cmake-vars.md) | profile 变量 · LLVM 专属开关 |

---

## 核心结论

1. 发行版 `llvm-dev` 等**二进制包**够用日常；**源码编译**才熟悉 LLVM 可定制性（加后端等）。  
2. **禁止内联构建** — 源码外独立 `build/` 目录。  
3. CMake + Ninja 是标准路径；`-DLLVM_ENABLE_PROJECTS=clang` 常一起编 Clang。  
4. 本仓库 IR 主线：**记录本机 LLVM/Clang 版本 + 能读 `.ll` 即可** — 详见 `notes/` 可选备忘。
