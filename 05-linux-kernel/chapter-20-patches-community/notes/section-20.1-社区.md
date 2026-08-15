## ① 社区 · The Community

| 论坛 | **LKML** — Linux Kernel Mailing List |
|------|--------------------------------------|
| 内容 | 公告、讨论、辩论、**新特性补丁** |

| 入门 | **订阅或读归档** — 观察真实开发节奏 |

→ 开篇：[Ch](../../chapter-01-intro/) · **GPL 协作开发**

| 现代补充 | 部分子系统迁 **lore.kernel.org** 列表 — 流程不变：**邮件 + 补丁** |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 如何向内核社区提交补丁？正确流程是什么？

<details><summary>答案</summary>

1) 读 MAINTAINERS 找到对应子系统维护者；2) `git format-patch` 生成补丁（标准格式）；3) `git send-email` 发到 LKML + 维护者 + 对应 mailing list；4) 回应 review 意见（v2/v3...）；5) 维护者 Acked-by/Reviewed-by 后收走 → 进子系统树 → 最终进 mainline。规则：一次只改一件事、commit message 清晰、通过 checkpatch.pl 检查。

</details>

</details>
---
