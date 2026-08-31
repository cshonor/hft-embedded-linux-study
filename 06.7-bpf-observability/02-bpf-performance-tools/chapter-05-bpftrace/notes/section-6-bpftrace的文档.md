# 5.6 bpftrace 的文档

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.6 节（印刷 p146）

## 内容详解

与 BCC 项目一致的工具文档体系：

| 文档 | 位置 | 内容 |
|------|------|------|
| man 手册 | `man/man8/` | 每工具一份（格式与用途同第 4 章所讲） |
| 示例文件 | `tools/*_example.txt` | 示例输出 + 输出解释 |
| 单行程序指引 | `docs/tutorial_one_liners.md` | **学习开发新单行/工具的入门教程** |
| 参考手册 | `docs/reference_guide.md` | 语言的完整参考（持续更新） |

作者为帮助学习专门编写了"bpftrace One-Liner Tutorial"和"bpftrace Reference Guide"两份文档，均在仓库 /docs 下。

### 四类文档的使用时机（别拿错工具）

| 场景 | 该查哪个 | 为什么 |
|------|---------|--------|
| "这个工具输出什么意思？" | man 8 / *_example.txt | 工具层问题 |
| "我想学写脚本，从哪开始？" | tutorial_one_liners.md | 教程按难度递进 |
| "hist() 参数怎么传？" | reference_guide.md | 语言层速查 |
| "为什么这句编译不过？" | reference_guide.md（**在线版**） | 语言演进快，书/本地版可能滞后 |

### 版本演进对照（书快照 v0.9-232 → 现代）

| 语言点 | 书中（2019, v0.9） | 现代版本趋势 |
|--------|-------------------|-------------|
| 循环 | 仅 `unroll(≤20)` | 内核 5.3+ 上支持有界 `for`/`while` |
| `kstack`/`ustack` | 基本用法 | 支持 `[kstack]` 内联文本、输出模式选项 |
| `str()` | 长度 64B（环境变量调） | 语义一致，上限仍受 BPF 512B 栈约束 |
| 地址空间 | 自动判定内核/用户 | `kptr()/uptr()` 显式区分（5.15 所述演进已落地） |
| 探针类型 | 12 类 | 新增 `watchpoint`、`asyncwatchpoint`、`kfunc`/`kretfunc`、`iter` 等 |

（具体以所装版本的 reference_guide 为准——这张表的正确用法是**提醒你别拿书当现行规范**，而不是背右边一列。）

## HFT 关联

- 团队新人入门路径建议：`tutorial_one_liners.md`（动手 1 小时）→ 本章 5.7–5.14（语法）→ `reference_guide.md`（查询手册）→ 附录 A（抄单行）；
- 参考手册在线持续更新，比书中内容新——**遇到语法报错先查在线版**（语言演进快：unroll→for/while、str 长度限制等都在变）；
- 交易机上没网时：`bpftrace --help` + 本地 docs/ 是兜底；离线 runbook 里固化单行时**同时固化它当时验证过的 bpftrace 版本号**，语法漂移的锅提前甩清楚。

## 陷阱

- ⚠️ 书中语法快照（2019 年 v0.9）与最新 bpftrace 有差异；以所装版本的 `reference_guide` 为准。
- ⚠️ 网上博客/Stack Overflow 的 bpftrace 片段要看出处年代——unroll 时代的写法在有 for/while 的版本上仍能跑，但反过来必然失败（老版本没有新语法）。

<details>
<summary>自测题</summary>

1. bpftrace 的两份核心学习文档是什么？
   <details><summary>答案</summary>"bpftrace One-Liner Tutorial"（单行程序指引）与"bpftrace Reference Guide"（参考手册），都在仓库 /docs 目录。</details>

2. 团队新人的推荐入门顺序是什么？
   <details><summary>答案</summary>tutorial_one_liners.md（动手）→ 5.7–5.14 语法 → reference_guide.md（工具书式查询）→ 附录 A 抄单行。</details>
</details>
