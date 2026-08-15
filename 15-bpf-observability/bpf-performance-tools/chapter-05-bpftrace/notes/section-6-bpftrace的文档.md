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

## HFT 关联

- 团队新人入门路径建议：`tutorial_one_liners.md`（动手 1 小时）→ 本章 5.7–5.14（语法）→ `reference_guide.md`（查询手册）→ 附录 A（抄单行）；
- 参考手册在线持续更新，比书中内容新——**遇到语法报错先查在线版**（语言演进快：unroll→for/while、str 长度限制等都在变）。

## 陷阱

- ⚠️ 书中语法快照（2019 年 v0.9）与最新 bpftrace 有差异；以所装版本的 `reference_guide` 为准。

<details>
<summary>自测题</summary>

1. bpftrace 的两份核心学习文档是什么？
   <details><summary>答案</summary>"bpftrace One-Liner Tutorial"（单行程序指引）与"bpftrace Reference Guide"（参考手册），都在仓库 /docs 目录。</details>
</details>
