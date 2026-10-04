# 4.1 BCC 的组件

> 底本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.1 节

## 内容详解

BCC（BPF Compiler Collection）是 BPF 的**主要前端项目**，仓库包含以下组件（图 4-1 目录结构）：

```
BCC/
├── tools/      # 70+ 开箱即用的性能工具（Python 脚本）
├── man/
│   └── man8/   # 每个工具一份 man 8 手册页
├── docs/       # 文档
├── src/        # 源码：C++ 前端 + 改写器 + 各语言前端（Python/C++/Lua）
├── examples/   # 示例
└── tests/      # 测试
```

| 组件 | 内容 | 使用者 |
|------|------|--------|
| `tools/` | 单用途/多用途工具脚本 | **最终用户**（直接跑） |
| `man/man8/` | 每工具一页手册（NAME/SYNOPSIS/DESCRIPTION/OVERHEAD/FIELDS/STABILITY） | 最终用户 |
| `src/` | libbcc、libbcc_bpf 库 + 改写器 + Clang/LLVM 驱动 | **工具开发者** |
| `examples/` | 用法示例 | 学习者 |

要点：

- **tools/ 与 src/ 面向不同人群**——绝大多数性能工程师只用 tools/，无需读 BCC 源码。
- 工具按主题分目录：`tools/*.py`（旧版平铺）与新版的 `lib/`（如 `libbpf-tools/`，BCC 仓库演化后 CO-RE 版本放这里）。
- man 手册与工具**一一对应**，这是 BCC 区别于"网上随手抄脚本"的工程化特征。

## tools/ 的两代实现（演进还在进行中）

同一个工具名在仓库里可能有两份实现，选型时要知道差异：

| | `tools/*.py`（BCC 版） | `libbpf-tools/*`（CO-RE 版） |
|---|---|---|
| 内核态语言 | C（运行时 Clang 编译） | C（**编译期**编好，BTF 重定位） |
| 启动速度 | 慢（每次现场编译，秒级） | 快（加载即跑） |
| 依赖 | 目标机要有内核 headers + LLVM | 只要 `/sys/kernel/btf/vmlinux` |
| 跨内核版本 | 脆（版本错位即编译失败/偏移错） | 稳（CO-RE 重定位） |
| 部署形态 | Python 脚本 | 静态链接单二进制 |

这是 BCC 仓库对"运行时编译三大代价"（见 [1.3](../../chapter-01-introduction/)）的自我修正——新工具默认走 CO-RE 路线，老 Python 版维护中。**新部署的生产观测，优先 libbpf-tools 版**。

## 工具的"三分天下"心智模型

70+ 工具不用背，按输出形态分三类就够：

| 类 | 命名特征 | 例子 | 开销模型 |
|---|---|---|---|
| 逐事件类 | `*snoop`、`*slower` | opensnoop、biosnoop、fileslower | ∝ 事件率（过滤后） |
| 聚合类 | `*latency`、`*count`、`*top`、`*dist` | biolatency、vfsstat、filetop | 与输出频率挂钩，事件率无关 |
| 采样类 | `profile`、`offcputime` | profile | ∝ 采样率 |

第一眼看到一个陌生工具名，先归类——类决定它能不能常驻、怎么用。

## HFT 关联

- 交易机上部署哪个目录？**只需要 tools/ 与 man/**；src/ 是开发期依赖，生产机不必装编译链（Clang/LLVM）——更彻底的做法是直接部署 libbpf-tools 的静态二进制，连 Python 都不用装。
- 固化巡检脚本时，版本锁定：BCC 工具随内核演进 API 有变动，升级内核前要回归测试 tools/ 清单；CO-RE 版的回归项换成"内核 BTF 存在性"检查。
- 三分天下模型直接映射部署策略：聚合类可常驻、采样类按需周期跑、逐事件类只排障。

## 陷阱

- ⚠️ 直接 `pip install bcc` 装到的可能是旧版本；发行版仓库（见 4.3）版本与内核匹配度更好。
- ⚠️ tools/ 下同名工具有多个实现（Python 版 / libbpf-tools 的 C 版），参数与开销略有差异，runbook 里要写清楚用的是哪个。
- ⚠️ `pip` 装的 BCC 和系统包装的 BCC 可能同时存在——`which opensnoop` 看到的不一定是 runbook 里指的那个，版本核对要看到 `-h` 输出的版本串。

<details>
<summary>自测题</summary>

1. BCC 仓库中最终用户最常使用的两个目录是什么？
   <details><summary>答案</summary>`tools/`（工具）与 `man/man8/`（手册页）。</details>

2. 为什么说 tools/ 和 src/ 面向不同人群？
   <details><summary>答案</summary>tools/ 是给性能工程师直接运行的脚本；src/ 是 libbcc/libbcc_bpf 等库与编译器前端，只有开发新工具的人才需要。</details>

3. tools/*.py 与 libbpf-tools/ 两代实现的本质差异是什么？新部署该选哪个？
   <details><summary>答案</summary>编译时刻：BCC 版运行时用 Clang 现场编译（依赖 headers、启动慢、跨版本脆），CO-RE 版编译期完成 + BTF 重定位（静态单二进制、秒级启动、跨版本稳）。新生产部署优先 libbpf-tools。</details>

4. 用"三分天下"归类：filetop、opensnoop、profile 各属哪类？各能否常驻生产？
   <details><summary>答案</summary>filetop 聚合类（定时快照，可常驻）；opensnoop 逐事件类（只排障/短窗口）；profile 采样类（可周期跑，采样率可控）。判断依据是开销模型而非工具名直觉。</details>
</details>
