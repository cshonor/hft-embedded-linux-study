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

## HFT 关联

- 交易机上部署哪个目录？**只需要 tools/ 与 man/**；src/ 是开发期依赖，生产机不必装编译链（Clang/LLVM）。
- 固化巡检脚本时，版本锁定：BCC 工具随内核演进 API 有变动，升级内核前要回归测试 tools/ 清单。

## 陷阱

- ⚠️ 直接 `pip install bcc` 装到的可能是旧版本；发行版仓库（见 4.3）版本与内核匹配度更好。
- ⚠️ tools/ 下同名工具有多个实现（Python 版 / libbpf-tools 的 C 版），参数与开销略有差异，runbook 里要写清楚用的是哪个。

<details>
<summary>自测题</summary>

1. BCC 仓库中最终用户最常使用的两个目录是什么？
   <details><summary>答案</summary>`tools/`（工具）与 `man/man8/`（手册页）。</details>

2. 为什么说 tools/ 和 src/ 面向不同人群？
   <details><summary>答案</summary>tools/ 是给性能工程师直接运行的脚本；src/ 是 libbcc/libbcc_bpf 等库与编译器前端，只有开发新工具的人才需要。</details>
</details>
