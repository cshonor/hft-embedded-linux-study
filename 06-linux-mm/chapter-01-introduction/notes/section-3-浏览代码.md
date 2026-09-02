# Ch 1 §3 浏览代码 (Browsing the Code)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`cscope` / `ctags` / `scripts/clang-tools/gen_compile_commands.py`）

---

## 本节讲什么

本节回答三个问题：

1. 内核函数**跨文件、跨 `arch/`、跨宏展开**，纯 `grep` 为什么不够、会漏什么？
2. `cscope` / `ctags` / clangd 三套工具**各自解决什么问题、命令怎么用**？
3. 什么是**调用图 (call graph)**，什么时候才需要它？

「会找定义、能跟调用链」是读 `mm/` 的元能力——没有它，后面 Ch2 起的所有源码都读不动。

---

## 1. 为什么 `grep` 不够

内核里同一个符号常有**多个定义**（宏 + 内联函数 + 不同 `arch/` 的弱符号），`grep` 只能告诉你「这名字在哪出现」，分不清**定义 vs 引用 vs 宏展开**：

```c
// mm/page_alloc.c 里想找 __alloc_pages 的所有"调用者"
grep -rn "__alloc_pages" mm/ | wc -l    # 可能几百行，混着定义、注释、文档、不同重载
```

真正的需求是：**点一下符号 → 跳到唯一定义；再查 → 列出所有真引用**。这就是交叉引用工具干的事。

---

## 2. 三套工具的分工

| 工具 | 定位 | 擅长 | 局限 |
|------|------|------|------|
| **cscope** | 本地交叉引用数据库 | 找「谁调用了我、我调用了谁」、找符号**所有引用** | 纯文本匹配，不理解类型/宏展开 |
| **ctags** | 标签跳转 | `vim` 里 `Ctrl-]` 秒跳**定义** | 只索引定义，查引用弱 |
| **clangd / LSP** | 语义级 IDE 后端 | **类型感知**：跳定义、找引用、补全、宏展开、诊断 | 需要 `compile_commands.json`，首次建库慢 |

---

## 3. cscope / ctags 实战

```bash
# 建索引（在源码树根）
cscope -R -b            # 递归扫 .c/.h/.S，生成 cscope.out 数据库
ctags -R .              # 生成 tags 文件

# cscope 查询（-d 免重建、-L 行模式、数字=查询类型）
cscope -d -L3 __alloc_pages      # 3=找所有引用
cscope -d -L0 __alloc_pages      # 0=找定义
cscope -d -L2 __alloc_pages      # 2=找被本符号调用的函数
cscope -d -L1 free_pages         # 1=找调用本符号的函数

# vim 里 ctags 跳转
vim mm/page_alloc.c
# 光标停在 __alloc_pages 上，Ctrl-] 跳到定义，Ctrl-T 跳回
```

查询类型速查（`cscope -d` 交互界面也有对应菜单）：

| 编号 | 查询 | 用途 |
|:----:|------|------|
| 0 | Find this C symbol | 找定义 |
| 1 | Find functions called by this function | 找「我调谁」（下钻） |
| 2 | Find functions calling this function | 找「谁调我」（上溯） |
| 3 | Find this text string / all references | 找所有出现 |

---

## 4. clangd：类型感知的现代路线

`cscope` 是**文本匹配**，遇到宏、重载、`static inline` 多个同名函数会「傻」。clangd 用**编译器前端**做语义分析，理解类型、宏展开、`#ifdef` 生效分支：

```bash
# 第一步：生成 compile_commands.json（关键！没有它 clangd 是瞎子）
scripts/clang-tools/gen_compile_commands.py   # 从 .cmd 文件重建每个编译单元的精确命令

# 第二步：clangd 读到 compile_commands.json 后
#   - 跳定义：精确到"此刻生效的那个定义"（不是 grep 命中的第一个）
#   - 找引用：只列"真的会执行到"的引用
#   - 宏展开：CONFIG_HIGHMEM 到底是 1 还是 0，直接看展开结果
```

**为什么需要 `compile_commands.json`？** 内核每个 `.c` 的编译命令都不同（不同 `-D`、`-I`、`arch` 下不同头文件）。clangd 必须知道「这个文件是按哪条命令编的」，才能还原出**正确的宏环境和头文件路径**。`gen_compile_commands.py` 就是把这信息从 `make` 留下的 `.cmd` 文件里挖出来。

**实际搭配建议**：日常用 **Elixir（在线）+ clangd（本地）** 双轨——Elixir 零配置、随时查；clangd 做**深挖调用链 + 看宏展开**。`cscope` 留着做「快速全仓引用」的兜底。

---

## 5. 调用图：什么时候才需要

原书的 **CodeViz**（作者为写书开发）生成**函数调用图**，一眼看子系统结构。它已经停止维护，现代替代：

| 工具 | 输出 | 适用 |
|------|------|------|
| **clangd call hierarchy** | 编辑器里展开「谁调我/我调谁」树 | 日常跟链 |
| **Doxygen + Graphviz** | 静态调用图（有向图） | 出文档、画架构 |
| **手工 `grep` + ASCII 图** | 精简版调用链 | **本仓库笔记的标准做法** |
| **ftrace / perf** | **运行时**真调用图 | 调性能、找热路径 |

**关键判断**：静态调用图在**「理解结构」**时有用，但内核调用链**运行时才定型**（函数指针、回调、per-CPU 路径），静态图常画出「不会发生的边」。所以**调性能时信 `ftrace`/`perf`，不信静态图**——这条对 HFT 尤其重要。

---

## 6. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 追一条延迟尖刺的完整路径 | clangd call hierarchy 下钻 + `ftrace` 运行时验证 |
| 判断 `CONFIG_XXX` 到底开没开 | clangd 宏展开（比翻 `.config` 更直接看生效分支） |
| 快速定位「谁调了 `__alloc_pages` 导致页分配抖动」 | `cscope -d -L2 __alloc_pages` 全仓上溯 |
| 理解 `mm/` 一个函数的作用 | 先 Elixir 跳定义，再查引用，别硬啃单个文件 |

---

## 7. 衔接

- 下节 [§4 阅读代码的策略](./section-4-阅读代码的策略.md)：工具备齐了，从哪个 FILE 开始读
- 源码管理：[§2 源码管理](./section-2-源码管理.md)（读的这棵树是怎么来的）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`cscope` 和 `ctags` 都是「跳转」，为什么要两个？**
A：分工不同。`ctags` 只做**「跳定义」**这一件事，轻量、快；`cscope` 做**「交叉引用」**——找谁调我、我调谁、所有引用。读内核「跟调用链」靠 cscope，vim 里日常「跳到函数定义」靠 ctags 更顺手。

**Q2：clangd 为什么必须要有 `compile_commands.json`，而 cscope 不用？**
A：clangd 是**语义分析**，必须知道每个文件的确切编译参数（宏定义、头文件路径、`arch`）才能还原真实代码，否则连 `#ifdef CONFIG_XXX` 都判断不了。cscope 是**纯文本匹配**，不需要编译上下文，但代价是不懂类型和宏。

**Q3：`cscope -d -L2` 和 `-L1` 查出来的方向相反，容易记混，怎么记？**
A：想象箭头。`-L2` = 「本函数**调用**了谁」→ 从本函数**出发往下**找；`-L1` = 「谁**调用**了本函数」→ **往上**找调用者。追「这函数被谁触发」用 1，追「这函数内部会走到哪」用 2。

**Q4：静态调用图（Doxygen 之类）为什么可能画出「不会发生的边」？**
A：因为很多调用是**函数指针 / 回调 / per-CPU 分派**，静态分析只能看到「这里有个指针可能指向 A、B、C」，于是把三条边都画出来，但运行时只有一条真发生。所以结构理解可以看静态图，**性能结论必须用 `ftrace`/`perf` 的运行时图验证**。

**Q5：读 `mm/` 时，Elixir 和本地 clangd 各什么时候用？**
A：**快速查一个符号**（「这个函数在哪定义」）用 Elixir，零配置、开网页即用；**深挖一条调用链、看宏展开、确认 `CONFIG` 分支**用本地 clangd，因为它有完整的编译上下文。两者不是替代关系，是「速查 vs 深挖」的分层。

</details>

---
