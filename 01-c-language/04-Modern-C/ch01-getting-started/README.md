# Ch1 · Getting started（入门）

> **Level 0 · 邂逅** · 策略：**⏭️ 跳过**（前三本书已覆盖）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

编译/链接/执行全流程、工具链初识、第一个程序。如果你已读完 K&R 和 CSAPP，这部分可跳过；
唯一值得关注的是 **C23 编译选项**和**现代构建工具链**的选择。

## 一、编译流水线

```
source.c → 预处理(.i) → 编译(.s) → 汇编(.o) → 链接(可执行)
   cpp        cc1         as           ld
```

| 阶段 | 做什么 | 对应 gcc 选项 |
|------|--------|--------------|
| 预处理 | 展开 `#include`、`#define`、条件编译 | `-E`（只预处理） |
| 编译 | 语法分析 → 优化 → 生成汇编 | `-S`（到汇编为止） |
| 汇编 | 汇编 → 目标文件（`.o`） | `-c`（到目标文件为止） |
| 链接 | 合并目标文件 + 库 → 可执行 | 默认（或 `-o name`） |

```bash
gcc -std=c2x -Wall -Wextra -Wpedantic -O2 -o hello hello.c
```

## 二、`-std=` 选项速查

| 选项 | 标准 | `__STDC_VERSION__` |
|------|------|---------------------|
| `-std=c89` / `-std=c90` | C89/C90 | 未定义（或 `199409L` 加 Amendment 1） |
| `-std=c99` | C99 | `199901L` |
| `-std=c11` | C11 | `201112L` |
| `-std=c17` / `-std=c18` | C17 | `201710L` |
| `-std=c2x` / `-std=c23` | C23 | `202311L` |

> `gnu` 前缀（如 `-std=gnu11`）= 标准 + GNU 扩展。内核和 DPDK 都用 `gnu` 变体。

## 三、现代构建工具

| 工具 | 用途 | HFT 项目中的角色 |
|------|------|-----------------|
| **make** | 基础构建 | DPDK Makefile / 内核 Kbuild |
| **CMake** | 跨平台构建 | 用户态 HFT 框架常用 |
| **meson** | 更快的构建 | DPDK 19.11+ 已迁移到 meson |
| **pkg-config** | 查找库 | `pkg-config --cflags libdpdk` |

## 四、调试器与工具链

| 工具 | 用途 |
|------|------|
| **gdb** | 调试器（HFT 常用 `gdb -p <pid>` attach 到运行中的进程） |
| **valgrind** | 内存检测（热路径太慢，只在测试用） |
| **perf** | 性能分析（见 06.6 Systems Performance） |
| **strace/ltrace** | 系统调用/库调用追踪 |

## HFT / DPDK 关联

前三本书 + CSAPP 已覆盖入门内容。回来查的场景：
- 确认某个 C23 特性需要哪个 gcc 版本（gcc 14+ 基本完整支持 C23）
- meson 构建参数（`-Dc_args='-std=c11'`）
- 编译期特性检测（`__STDC_VERSION__` 宏）

## 自测题

<details><summary>1. 如何让 gcc 只输出预处理结果？</summary>

`gcc -E hello.c`。常用于排查宏展开问题——HFT 项目里宏嵌套深时必备。
</details>

<details><summary>2. <code>__STDC_VERSION__</code> 在 <code>-std=c11</code> 下的值是多少？</summary>

`201112L`。可用 `#if __STDC_VERSION__ >= 201112L` 做条件编译，在新旧标准间切换写法。
</details>
