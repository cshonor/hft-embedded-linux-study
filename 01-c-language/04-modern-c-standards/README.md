# 《Modern C》（第三版 · C23）

**Modern C, Third Edition** — Jens Gustedt（ISO C 标准委员会成员）

> **第 4 本书** · C99–C23 标准增量 + 内存模型 / 原子并发（标准 C 收官）

> 免费版（CC BY-NC-ND）：`https://gustedt.gitlabpages.inria.fr/modern-c/`（2024-10 更新，覆盖 C23）  
> 纸质版：Manning 2025-09（第三版，ISBN 9781633437777）

## 定位

**标准增量 + 现代写法**。前三本书（K&R + 指针 + 专家编程）覆盖 C89 基础后，
本书把 C99–C23 的增量补齐，并以标准委员会视角重讲指针、内存模型、原子与并发——
**进 DPDK 前的理论收官**。

为什么不换掉前三本书：内核主体仍是 GNU C89/C99，前三本书教的是标准无关的思维；
Modern C 默认你已会 C89，不从头教语法，正好无缝衔接。

## 阅读优先级（HFT / DPDK 方向）

| 章 | 策略 | 理由 |
|----|------|------|
| Ch1–3, 8–10, 14 | ⏭️ 跳过/字典 | 前三本书已覆盖 |
| Ch4–7 | 🟡 略读 | 只看 C23 增量：constexpr、`{}` 初始化、inline 新规则、_BitInt |
| Ch11 指针 | 🟡 对照速读 | 《C 和指针》已深讲；只看 opaque struct 封装 + C23 nullptr |
| **Ch12 C 内存模型** | 🔴 精读 | effective type（能否直接 cast 网络字节）、对齐 alignas |
| **Ch13 存储** | 🔴 精读 | 四种存储期、malloc 替代思路（HFT 内存池的理论基础） |
| Ch15 程序失败 | 🟡 略读 | 错误处理系统化，与内核 goto err 同思想 |
| **Ch16 性能** | 🔴 精读 | **restrict 别名消除**（零成本优化，热路径必用）、inline |
| Ch17–18 | 🟡 略读 | 宏与 _Generic/typeof（内核宏早已用 typeof） |
| Ch19 控制流 | 🟡 略读 | **signal handler 的 async-signal-safe 限制**要看 |
| **Ch20 线程** | 🔴 精读 | **进 13 DPDK 前必读**：每 lcore 一线程 = _Thread_local 模型 |
| **Ch21 原子与内存一致性** | 🔴 精读 | **DPDK rte_ring 的理论基础**：happens-before、五种内存序 |
| 附录 A/B/C | 🟡/⏭️ | 迁移技巧 + 编译器/库对照表 |

**一句话路线：** 前三本 → 00 差异速查 → Ch12/13/16 → （开 DPDK 前）Ch20/21 → 其余当字典。

## 章节索引

全书按 4 个 Level 组织（作者的教学层级，不是难度，是"对语言的理解深度"）。

### Level 0 · 邂逅（Encounter）

| 章 | 目录 | 主题 | 策略 |
|----|------|------|------|
| 1 | [ch01-getting-started](./ch01-getting-started/) | 入门 | ⏭️ |
| 2 | [ch02-principal-structure-of-program](./ch02-principal-structure-of-program/) | 程序的主要结构 | ⏭️ |

### Level 1 · 相识（Acquaintance）

| 章 | 目录 | 主题 | 策略 |
|----|------|------|------|
| 3 | [ch03-everything-about-control](./ch03-everything-about-control/) | 一切都与控制有关 | ⏭️ |
| 4 | [ch04-expressing-computations](./ch04-expressing-computations/) | 表达式的计算 | 🟡 |
| 5 | [ch05-basic-values-and-data](./ch05-basic-values-and-data/) | 基本值和数据 | 🟡 |
| 6 | [ch06-derived-data-types](./ch06-derived-data-types/) | 派生数据类型 | 🟡 |
| 7 | [ch07-functions](./ch07-functions/) | 函数 | 🟡 |
| 8 | [ch08-c-library-functions](./ch08-c-library-functions/) | C 标准库函数 | ⏭️ |

### Level 2 · 相知（Cognition）

| 章 | 目录 | 主题 | 策略 |
|----|------|------|------|
| 9 | [ch09-style](./ch09-style/) | 风格 | 🟡 |
| 10 | [ch10-organization-and-documentation](./ch10-organization-and-documentation/) | 组织与文档 | 🟡 |
| 11 | [ch11-pointers](./ch11-pointers/) | 指针 | 🟡 |
| **12** | [ch12-c-memory-model](./ch12-c-memory-model/) | **C 内存模型** | 🔴 |
| **13** | [ch13-storage](./ch13-storage/) | **存储** | 🔴 |
| 14 | [ch14-more-involved-processing-io](./ch14-more-involved-processing-io/) | 更复杂的处理与 IO | ⏭️ |
| 15 | [ch15-program-failure](./ch15-program-failure/) | 程序失败 | 🟡 |

### Level 3 · 深入（Experience）

| 章 | 目录 | 主题 | 策略 |
|----|------|------|------|
| **16** | [ch16-performance](./ch16-performance/) | **性能** | 🔴 |
| 17 | [ch17-function-like-macros](./ch17-function-like-macros/) | 类函数宏 | 🟡 |
| 18 | [ch18-type-generic-programming](./ch18-type-generic-programming/) | 类型泛型编程 | 🟡 |
| 19 | [ch19-variations-in-control-flow](./ch19-variations-in-control-flow/) | 控制流的变化 | 🟡 |
| **20** | [ch20-threads](./ch20-threads/) | **线程** | 🔴 |
| **21** | [ch21-atomic-access-memory-consistency](./ch21-atomic-access-memory-consistency/) | **原子访问与内存一致性** | 🔴 |

### 附录

| 附录 | 目录 | 主题 | 策略 |
|------|------|------|------|
| A | [appendix-a-transitional-code](./appendix-a-transitional-code/) | 过渡代码 | 🟡 |
| B | [appendix-b-c-compilers](./appendix-b-c-compilers/) | C 编译器 | ⏭️ |
| C | [appendix-c-c-libraries](./appendix-c-c-libraries/) | C 库 | ⏭️ |

## 前置速查

- [00-C89-to-C23-diff-and-reading-map](./00-C89-to-C23-diff-and-reading-map.md) — C89→C23 差异速查 + 其余五本书"过时清单"（先读这个再开书）

## 学习进度（推荐阅读顺序）

> 以下为 Claw 推荐的精读路线，按顺序逐章推进：

- [ ] **① 00 差异速查** — C89→C23 增量一览，先建立全局视角
- [ ] **② Ch12 C 内存模型** — effective type、严格别名、对齐（接 03 的 ch06/07）
- [ ] **③ Ch13 存储** — 四种存储期、malloc 替代（HFT 内存池理论）
- [ ] **④ Ch16 性能** — restrict 别名消除、inline（热路径必用）
- [ ] **⑤ Ch20 线程** — C11 线程、_Thread_local（进 DPDK 前必读）
- [ ] **⑥ Ch21 原子与内存一致性** — happens-before、五种内存序（DPDK rte_ring 理论基础）
- [ ] 📖 其余 🟡 章节按需查阅，不要求顺序读
