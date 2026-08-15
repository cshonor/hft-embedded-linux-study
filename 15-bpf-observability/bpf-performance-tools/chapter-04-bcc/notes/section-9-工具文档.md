# 4.9 工具文档

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.9 节

## 内容详解

BCC 70+ 工具全部带标准文档：**man 8 手册页 + 示例文件**。这是 BCC 生态"工程化"的核心特征，也是本书反复强调的使用习惯。

### man 8 手册页标准结构

| 段 | 内容 | 为什么要读 |
|----|------|-----------|
| **NAME** | 一句话定位 | 快速判断工具是否对口 |
| **SYNOPSIS** | 命令行语法 | 参数全貌 |
| **DESCRIPTION** | 原理：挂哪个 probe、内核里做什么聚合 | 理解输出含义 |
| **OVERHEAD** | 开销估算 | **能否常驻生产的依据** |
| **FIELDS** | 输出字段逐个解释 | 读懂每一列 |
| **STABILITY** | 稳定性：接口是否随内核变动 | 升级内核的风险评估 |
| **EXAMPLES** | 常用命令 | 抄作业 |

### opensnoop 的 example 文件规范

`examples/tracing/opensnoop_example.txt` 给出从**默认用法到排障场景**的渐进示例：基本跟踪、按 PID、只看失败（`-x`）、带时间戳、统计分布等——每个示例一句话说明用途。

### 使用习惯（本书主张）

1. 遇到新工具：先 `man 8 <tool>` 读 DESCRIPTION + OVERHEAD；
2. 再看 example 文件抄最接近的命令；
3. 输出对不上预期时回看 FIELDS。

## HFT 关联

- 团队 runbook 中固化任何 BCC 工具前，**必须**把 man 的 OVERHEAD 与 STABILITY 段抄进评审记录——这是合规要求（变更可解释）也是容量要求（开销可预算）。
- STABILITY 段直接决定"内核小版本升级时哪些巡检脚本要回归"。

## 陷阱

- ⚠️ 发行版包装的 man 页可能与工具版本不匹配（man 是旧的）——以 `tool -h` 的 USAGE 输出为最终准绳。
- ⚠️ 没读 OVERHEAD 就把工具挂上生产机，是最常见的"观测引入故障"。

<details>
<summary>自测题</summary>

1. man 8 手册哪一段决定工具能否常驻生产？
   <details><summary>答案</summary>OVERHEAD（开销估算）。</details>

2. STABILITY 段回答什么问题？
   <details><summary>答案</summary>工具依赖的内核接口（kprobe 等）是否稳定，升级内核时是否会失效。</details>
</details>
