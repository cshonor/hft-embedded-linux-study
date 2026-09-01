## ⑥ 内核开发社区

| 要点 | 说明 |
|------|------|
| **参与** | 全球协作 · 读 **邮件列表** · 提 **补丁** |
| **LKML** | **Linux Kernel Mailing List** — 主论坛 · **Linus 与维护者** 在场 |
| **学习法** | **读源码 + 动手改** — 优于只啃概念 |

#### 维护者金字塔（补丁怎么爬到 Linus 手里）

```
            Linus（唯一 merge 权）
              ▲  pull 子系统树
     ~百名 subsystem maintainer（net/sched/mm/xfs/...）
              ▲  pull driver 树
     数千名 driver maintainer（你司的网卡驱动维护者在这层）
              ▲  patch
     贡献者（你我）—— 代码 + commit log + review 意见
```

| 环节 | 规则 |
|------|------|
| 提交格式 | `./scripts/checkpatch.pl` 过检；commit log 说**为什么** |
| review 文化 | **粗鲁但只对代码**——被 nit 挑刺是流程的一部分 |
| merge window | 每个 rc1 后两周；错过等下个周期（约 10 周） |
| Signed-off-by | 每位经手人签名（DCO）——法律链条 |

#### 为什么这套「史前」流程赢了

| 特征 | 价值 |
|------|------|
| 邮件列表（而非 GitHub PR） | 补丁即邮件——**可 grep 的永久档案**、可离线 review、无平台锁定 |
| 维护者层级 | 逐级过滤：Linus 只看子系统树，吞吐量才撑得起每 9 周一版 |
| 无强中心化代码托管 | git 分布式——任何镜像都是完整历史 |

#### 贡献者工具箱（把补丁发出去的实操链路）

| 工具 | 用途 |
|------|------|
| `scripts/get_maintainer.pl` | 对着补丁自动列出该子系统的 maintainer + 邮件列表（从 MAINTAINERS 文件推导）——**发给谁**的问题它说了算 |
| `scripts/checkpatch.pl` | 编码风格 + 常见错误静态检查；发补丁前不过检大概率被直接打回 |
| `b4`（b4.am) | 现代邮件补丁工作流：从邮件列表拉 patchset、生成 mbox、回信带 `b4 rm` 引用——社区自己在用的工具 |
| lore.kernel.org | 所有内核邮件列表的**永久归档 + 可检索**——找历史讨论语境的第一入口 |
| Patchwork | 子系统维护者跟踪 patch 状态（New/Accepted/Rejected）的看板——查你的补丁死没死 |

> 顺序感：`get_maintainer.pl` 找人 → `checkpatch.pl` 过检 → `git format-patch` 生成补丁 → `git send-email` 发出 → 在 lore/Patchwatch 上跟踪。整套流程零平台依赖，全命令行可脚本化——这正是邮件流派三十年的护城河。

→ 本书收官：[Ch 20 补丁与社区](../../chapter-20-patches-community/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** LKML（Linux Kernel Mailing List）在内核开发中扮演什么角色？

<details><summary>答案</summary>

LKML 是内核开发的唯一官方论坛。所有补丁必须发到 LKML 接受 review，Linus 和子系统维护者在此决策。HFT 工程师如果发现内核 bug 或需要新特性，正确路径是：读源码确认问题 → 写补丁 → 发 LKML → 回应 review → 通过 maintainer 进入 mainline。

</details>

**Q2.** 为什么「读源码 + 动手改」比只啃概念书更有效？

<details><summary>答案</summary>

内核代码是最终的 ground truth。LKD 书基于 2.6 内核，现代 5.x/6.x 已大改（如 CFS→EEVDF 调度器、io_uring、BPF）。只读书会停留在过时认知；读源码能发现实际数据结构和算法，改代码能验证理解是否正确。

</details>

**Q3.** 一个外公司工程师想让某内核子系统接受自己的补丁，标准路径是什么？为什么「先发 LKML 大方案」往往适反？

<details><summary>答案</summary>

标准路径：读 MAINTAINERS 找到**子系统维护者**→ 小步补丁 + 完整 commit log（说清为什么）→ 发该子系统邮件列表（不一定是 LKML 全场）→ 回应 review、改版（v2/v3…带 diff 注记）→ 维护者收进子系统树 → 下个 merge window 进主线。大方案先行的坑：RFC 大重构没有**小步可 review 的切分**，维护者无从下手；且没在邮件列表混过、不了解子系统现有讨论语境的"外来大方案"历史上几乎全被搁置。社区默契：**先修几个小 bug 建立信任，再提大设计**。

</details>

</details>
---
