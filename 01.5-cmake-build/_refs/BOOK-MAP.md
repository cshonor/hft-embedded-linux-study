# BOOK-MAP · 工具书映射

本模块是任务节点式，书不逐章精读，按节点需要查。

| 书 | 定位 | 服务哪些节点 |
|---|------|--------------|
| 《CMake构建实战：项目开发卷》 | 中文入门：从 C 项目构建基础讲起，搭配实例讲语法 | 01 / 02 / 03 通读，其余按需 |
| *Professional CMake: A Practical Guide*（Craig Scott） | 英文权威参考：modern CMake 立场最正，按专题查 | 02 查 Toolchain Files；03 查 libraries；05 查 compiler/linker options |

## 阅读路线

1. 中文版前几章（构建基础 + 语法）快速过——别恋战
2. **立刻拿 `STM32-/labs/00-toolchain-clang` 练手**（节点 01），CMake 光看不练等于没学
3. 《Professional CMake》只查专题，重点 Toolchain Files 一章（裸机刚需）

## 已知教材坑

- 大量教程/示例是老式写法：`include_directories`、`${CMAKE_SOURCE_DIR}`、全局 flag
- 甄别原则见模块 README：一切皆 target、属性挂在 target 上
