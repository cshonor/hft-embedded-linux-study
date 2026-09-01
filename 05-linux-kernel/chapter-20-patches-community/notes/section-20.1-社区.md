## ① 社区 · The Community

| 论坛 | **LKML** — Linux Kernel Mailing List |
|------|--------------------------------------|
| 内容 | 公告、讨论、辩论、**新特性补丁** |

| 入门 | **订阅或读归档** — 观察真实开发节奏 |

→ 开篇：[Ch](../../chapter-01-intro/) · **GPL 协作开发**

| 现代补充 | 部分子系统迁 **lore.kernel.org** 列表 — 流程不变：**邮件 + 补丁** |

---

### 一条反直觉的事实：内核没有「PR」

GitHub 时代的第一直觉是"提个 pull request"。内核没有这套东西，
**所有变更都是邮件**。这不是守旧，而是流程设计的必然结果：

| 维度 | GitHub 流程 | 内核流程 |
|------|------------|---------|
| 评审单元 | 一个 PR（N 个 commit 混在一起） | **一封邮件 = 一个逻辑变更** |
| 载体 | 网页富文本 + diff 渲染 | **纯文本邮件，补丁 inline 在正文里** |
| 讨论记录 | 关掉 PR 后基本不可检索 | **lore.kernel.org 永久归档，Message-Id 就是 URL** |
| 历史痕迹 | squash merge 后只剩一行 message | 每个 commit 都带 `Link:` 指回当年讨论 |
| 谁说了算 | 仓库 owner | MAINTAINERS 里的子系统维护者（见 [20.3](./section-20.3-指挥链.md)） |

> `Documentation/process/5.Posting` 的原文要求：
> *"Patches should always be sent as plain text. Please do not send them as
> attachments; that makes it much harder for reviewers to quote sections of
> the patch in their replies."*
> —— **禁止用附件**，因为评审要能逐行引用回复。

---

### 三层结构：列表 / 子系统列表 / lore 归档

```
你的补丁
   │  To: 子系统维护者（MAINTAINERS 的 M: 字段）
   │  Cc: 子系统列表（L: 字段）+ 相关开发者
   ▼
子系统列表（netdev@ / bpf@ / linux-mm@ …）── 抄送 LKML
   │  回复用 In-Reply-To 串成一条 thread
   ▼
lore.kernel.org 归档（每封邮件一个永久 URL）
   │  维护者 apply → 子系统树 → linux-next → mainline
   ▼
v6.x 发布 → stable 团队继续维护（见 [20.5](./section-20.5-补丁.md)）
```

**LKML 不是唯一的列表**，而且**大多数补丁的"主场"是子系统列表**。
v6.6 `MAINTAINERS` 实证（字段含义见 20.3）：

| 列表 | 覆盖 | 补丁跟踪（MAINTAINERS 的 `Q:` 字段） |
|------|------|-----------------------------------|
| `netdev@vger.kernel.org` | 网络协议栈 + 网卡驱动 | `patchwork.kernel.org/project/netdevbpf/list/` |
| `bpf@vger.kernel.org` | eBPF 核心与 verifier | 同一站点，带 `?delegate=` 视图 |
| `linux-mm@kvack.org` | 内存管理 | — |
| `linux-rt-users@vger.kernel.org` | PREEMPT_RT 的使用与讨论 | — |
| `regressions@lists.linux.dev` | **回归**专用通道（见 [20.4](./section-20.4-提交错误报告.md)） | — |

> 查"我这段改该发哪儿"，**永远以 `MAINTAINERS` 的 `M:` / `L:` 字段为准**，
> 不要凭印象。用 `scripts/get_maintainer.pl` 自动生成（见 20.3）。

---

### lore.kernel.org：内核的「第二历史」

LKML 从 2018 年起有了官方归档 **lore**（public-inbox 格式），这件事的影响被低估了：

| 能力 | 怎么用 |
|------|--------|
| 永久链接 | `https://lore.kernel.org/r/<Message-Id 去掉尖括号>` |
| 补丁自带讨论入口 | 提交时写 `Link: https://lore.kernel.org/r/...`，**submitting-patches 明确要求优先用 lore** |
| 直接把邮件变成分支 | `b4 am <msgid>` → 把整个 series 落成 git 提交；`b4 shazam` 连 tag 一起带上 |
| 批量补签名 | `b4 ty` 自动收集 thread 里的 `Reviewed-by:` / `Acked-by:` |

> 这带来一个工程上非常值钱的后果：
> **内核里任意一个提交，你都能查到它当初为什么这么写、谁反对过、反对理由是什么。**
> 这是 "squash merge + 一行 message" 流程给不了的。
> 排查"为什么这行代码长这样"时，`git log -1` 里的 `Link:` 就是入口。

---

### 邮件礼仪：会被真的扣分的几条

| 规则 | 为什么这条是硬的 |
|------|-----------------|
| **纯文本，禁 HTML** | HTML 邮件会把补丁包进 MIME，评审端看到的是一团 quoted-printable |
| **补丁 inline，不做附件** | 评审要逐行引用回复（5.Posting 明文规定） |
| **别让邮件客户端折行 / 改空白** | 折过的补丁**打不上**，直接被跳过。"先发给自己确认完整"是文档里的原话 |
| **底部引用 + trim** | 长 thread 里别人要能读懂；top-posting 在内核列表上非常不受欢迎 |
| 修 bug 且想进 stable | 抄送 `stable@vger.kernel.org`，并在补丁里写 `Cc: stable@vger.kernel.org` |

> 5.Posting 的金句：*"If there is any doubt at all, mail the patch to yourself
> and convince yourself that it shows up intact."*
> 详细的客户端配置见 `Documentation/process/email-clients.rst`
> （mutt / Thunderbird / Evolution / git send-email 都有专门章节）。

---

### 行为准则：CoC 是正式文件，不是口号

v6.6 树里有 `Documentation/process/code-of-conduct.rst`（Contributor Covenant 版）。
内核历史上以"技术争论极其直白"闻名，CoC 落地之后边界很清楚：
**对代码可以毫不留情，对人不行。**

| 可以 | 不可以 |
|------|--------|
| "这个改动会破坏 X 场景的语义" | "你根本没看懂这段代码" |
| "NACK，理由如下三条" | 人身攻击、嘲讽经验 |
| 直接指出回归风险 | 在 thread 里跑题吵架 |

---

### 发布节奏：为什么「我提的补丁两周没动静」

`Documentation/process/2.Process` 实证的节奏：

```
合并窗口 merge window（约 2 周）
   │  新特性、大改动；日合入量接近 1000 个变更
   │  ▸ 这些改动不是凭空来的，是提前在子系统树里攒好的
   ▼ Linus 宣布窗口关闭
vX.Y-rc1
   │  6~10 周：只收修复（fixes）
   ▼
vX.Y 正式发布 ──► 交给 stable 团队长期维护
```

> **错过合并窗口的新特性，只能等下一个开发周期**（约 2~3 个月）。
> 文档里唯一的例外是**全新硬件的驱动**——它不碰树内代码，造不出回归，
> 所以任何时间都能进。这条"例外"本身就在告诉你内核最怕什么：**回归**。

---

### HFT 视角：为什么值得订阅

| 理由 | 具体做法 |
|------|---------|
| **你依赖的行为先在这里改** | 调度器、网络栈收包路径、RT 抢占的改动都先在列表上吵完，才进发行版内核 |
| **延迟回归有人先踩** | `regressions@lists.linux.dev` + regzbot 的周报，是免费的"内核版本避雷清单" |
| **定位问题能查到动机** | 通过 `git log` 的 `Link:` 回到 lore，直接读到当年 reviewer 的顾虑 |
| **低成本跟进方式** | 别订阅全量 LKML（量太大）；订阅 netdev / bpf / linux-rt-users，或用 lore 的 Atom feed + `b4` |

> 对 HFT 团队最实际的收益：**升级内核版本前，先看这个版本的回归清单和 netdev 的
> 收包路径改动**，比上线后追延迟毛刺便宜得多。

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 如何向内核社区提交补丁？正确流程是什么？

<details><summary>答案</summary>

1) 读 MAINTAINERS 找到对应子系统维护者；2) `git format-patch` 生成补丁（标准格式）；3) `git send-email` 发到 LKML + 维护者 + 对应 mailing list；4) 回应 review 意见（v2/v3...）；5) 维护者 Acked-by/Reviewed-by 后收走 → 进子系统树 → 最终进 mainline。规则：一次只改一件事、commit message 清晰、通过 checkpatch.pl 检查。

</details>

**Q2.** 为什么内核坚持用邮件而不是 GitHub PR？

<details><summary>答案</summary>

不是守旧，是三条硬需求：① **评审粒度**——一封邮件 = 一个逻辑变更，reviewer 可以逐条 NACK 其中的某一封，PR 里 N 个 commit 混在一起做不到；② **可检索的永久记录**——lore.kernel.org 用 Message-Id 做永久 URL，每个 commit 用 `Link:` 标签指回讨论，几万封邮件的 thread 依然可查；③ **离线工作流**——纯文本邮件可以用任何 MUA（mutt、git send-email）处理，社区里大量开发者就是在终端里工作的。补丁 inline 而非附件也是同一逻辑：评审要能**逐行引用回复**。

</details>

**Q3.** 补丁提交后两周没有任何回复，最可能的原因是什么？

<details><summary>答案</summary>

按概率排序：① **错过合并窗口**——新特性只能在约 2 周的 merge window 内合入，rc 期间只收修复，错过就要等下一个周期（文档原文：*"if you miss the merge window for a given feature, the best thing to do is to wait for the next development cycle"*）；② **发错地方**——没抄到正确的子系统列表/维护者，用 `scripts/get_maintainer.pl` 核对；③ **补丁本身被打回**——邮件客户端折行、HTML 格式、附件形式，这类邮件通常直接被忽略；④ 只是忙——子系统维护者是志愿者，可以礼貌 ping（一般在 v2 发送时顺带说明）。

</details>

**Q4.** lore.kernel.org 在实际排障中怎么用？

<details><summary>答案</summary>

三步：① 在 `git log -1 <commit>` 里找 `Link:` 标签（内核要求引用邮件归档时优先用 lore）；② 打开 `https://lore.kernel.org/r/<Message-Id>` 读当年的完整 thread，包括 reviewer 提过的反对意见和没解决的问题——这些**不会写进 commit message**；③ 用 `b4 am <msgid>` 把整个补丁系列落成 git 提交，本地复现/对比。`b4 ty` 还能自动收集 thread 里的 `Reviewed-by:` / `Acked-by:` 标签。这是"为什么这行代码长这样"这个问题的标准答案来源。

</details>

</details>
---
