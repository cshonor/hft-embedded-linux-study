## ⑤ 补丁 · Patches

**所有内核修改以补丁形式流通** — 社区的通用语言。

#### 生成补丁

| 方式 | 命令/工具 |
|------|-----------|
| 经典 | **`diff -urN`** 对比 **未改树** vs **修改树** |
| **推荐** | **Git** |

```bash
# Git 工作流（书中 + 现代常态）
git commit -a -m "net: fix foo in bar driver"
git format-patch -1          # 为最近 1 个提交生成 0001-*.patch
git format-patch origin/main # 相对主线的一系列补丁
```

→ **Ch 2** `git clone` · `patch -p1` · **Ch 18** `git bisect`

#### 提交补丁

| 项 | 规范 |
|----|------|
| **收件人** | 相关 **维护者** + **抄送列表** |
| **主题** | **`[PATCH] brief description`** |
| **正文** | **技术说明 + 理由** · **基于的内核版本** |
| **形式** | 补丁 **inline 纯文本** 附在邮件末尾 |
| 大改动 | **拆成多个逻辑独立小补丁** |
| 提交后 | **耐心** · 回应 review · **发修订版** `[PATCH v2]` |

```
邮件结构：
  To: maintainer@...
  Cc: linux-kernel@vger.kernel.org
  Subject: [PATCH] net: fix race in foo

  说明段落（why / what / testing）

  ---
  diff --git a/...
  （补丁正文 inline）
```

| HFT 团队 | 内部驱动/内核 fork 仍建议 **小步 commit + format-patch 风格说明** — 便于 audit 与回滚 |

---

### 第一原则：一封邮件 = 一个逻辑变更

`submitting-patches.rst` 有一整节叫 **"Separate your changes"**。判断标准：

| 情形 | 做法 |
|------|------|
| 既修 bug 又做性能优化（同一驱动） | **拆成两个补丁** |
| 既改 API 又加新用户 | 先改 API（及所有用户），再单独加新用户 |
| 大重构 | 拆成"机械改写" + "语义改动"两批，便于 reviewer 分别验证 |

> 原因很实际：**reviewer 可以逐条 NACK 系列里的某一封**，
> 混在一起的补丁没法部分接受，也没法 `git revert` 单个逻辑变更。

---

### canonical patch format 全解（v6.6 实证）

文档 `submitting-patches.rst` 里 *"The canonical patch format"* 一节的结构：

```
Subject: [PATCH 001/123] subsystem: summary phrase
                                                 ← 空行
From: Patch Author <author@example.com>           ← 仅当发送者 ≠ 作者时需要
                                                 ← 空行
解释正文，按 75 列折行                             ← 会被永久写进 changelog
（说清 what + why，可附 oops / 日志症状）
                                                 ← 空行
Signed-off-by: Author <author@example.com>
Fixes: 54a4f0239f2e ("KVM: MMU: make kvm_mmu_zap_page() return ...")
Link: https://lore.kernel.org/r/...
Cc: stable@vger.kernel.org
                                                 ← 分隔符行：---
不适合进 changelog 的内容（版本变更说明、range-diff…）
                                                 ← 空行
diff --git a/... （补丁正文）
```

| 部分 | 要点 |
|------|------|
| **`---` 分隔符之上** | **进 changelog**，永久留存 |
| **`---` 分隔符之下** | **不进 changelog**，评审用的临时信息（v2 改了什么、移除谁的 tag） |

---

### Subject 的硬规则

```
Subject: [PATCH 001/123] subsystem: summary phrase
          └─ 标签 ─┘  └─ 子系统 ─┘ └─ 70~75 字符 ─┘
```

| 规则 | 出处/理由 |
|------|----------|
| `subsystem:` 前缀标明改动区域 | 如 `net:`、`mm:`、`bpf:`、`x86:` |
| **summary ≤ 70~75 字符** | 原文：*"the summary must be no more than 70-75 characters"* |
| 必须同时说清 **what + why** | 原文：*"describe both what the patch changes, as well as why the patch might be necessary"* |
| 序列号**零填充**（`001/123`） | 让**文本排序 == 数字排序**，任何邮件客户端按主题排序都对 |
| 标签放方括号 | `v2`、`RFC`、`1/4`；标签**不算** summary 的一部分 |

> **为什么 summary 要写得这么讲究**（原文理由）：
> 它会一路传播到 `git` changelog 里，成为这个补丁的**全局唯一标识符**；
> 几周后别人用 `gitk` / `git log --oneline` 扫上千个补丁时，
> **summary 常常是他们唯一会看的东西**，还会拿它去搜当年讨论。

---

### 标签字典（v6.6 实证）

| 标签 | 语义 | 注意 |
|------|------|------|
| **`Signed-off-by:`** | **DCO 1.1**（Developer's Certificate of Origin）声明你有权以该开源许可提交 | **必带**，否则补丁无法合入 |
| **`Fixes:`** | 指明补丁修的是哪个历史提交引入的问题 | 用 **SHA-1 前 12 位 + 单行摘要**，**不折行**（tag 豁免 75 列规则，方便脚本解析）；帮 stable 团队判断该回传到哪些版本 |
| **`Link:`** | 指向相关讨论/归档 | 用邮件归档时**优先 lore** |
| **`Closes:`** | 指向 bug 报告 URL | `Reported-by:` 后面**通常要跟**一个 `Closes:`（报告不在网上时除外）；**禁止**私有 tracker 和无效 URL |
| **`Reported-by:`** | 致谢报告者 | **私密渠道报告的，要先征得对方同意** |
| **`Tested-by:`** | 声明该补丁已被成功测试 | |
| **`Reviewed-by:`** | reviewer 的四条声明（见下） | 是**意见陈述**，不是保证 |
| **`Suggested-by:`** | 主意来自某人 | 若未在公开场合提出，要先征得同意 |
| **`Co-developed-by:`** | 共同作者 | ⚠️ **必须紧跟该共同作者的 `Signed-off-by:`**（原文：*every Co-developed-by: must be immediately followed by a Signed-off-by: by the associated co-author*） |
| **`Acked-by:`** | 认可，但**不如 `Reviewed-by:` 正式** | 常用于**受影响代码**的维护者；**不代表认可整个补丁**（跨子系统时可只 ack 自己那部分） |
| **`Cc: stable@vger.kernel.org`** | 请求回传稳定版 | 见下节 |

> **`Reviewed-by:` 的四条声明**（Reviewer's statement of oversight，原文）：
> (a) 做过技术评审；(b) 问题已反馈给提交者且对其回应满意；
> (c) 认为此刻这个改动**值得进内核**、且**没有已知会阻碍合入的问题**；
> (d) **不作任何担保**。
> 最后一条很关键：**这是个"意见"，不是"背书"。**

> **版本演进时的处理**（原文）：收到 `Tested-by` / `Reviewed-by` 后，
> 作者应在**下一版**里加上；但如果后续版本改动太大，这些 tag **不再适用就必须移除**，
> 且**移除要在 `---` 之后的 changelog 里说明**。

---

### `Fixes:` 与稳定版回传

```bash
# 文档里教的小技巧：git 直接生成 Fixes: 行
git config --global core.abbrev 12
git config --global pretty.fixes 'Fixes: %h ("%s")'
git log -1 --pretty=fixes 54a4f0239f2e
# → Fixes: 54a4f0239f2e ("KVM: MMU: make kvm_mmu_zap_page() return the number of pages it actually freed")
```

⚠️ 原文明确：**加了 `Fixes:` 不等于自动进 stable**——
*"Attaching a Fixes: tag does not subvert the stable kernel rules process nor
the requirement to Cc: stable@vger.kernel.org"*。

**`-stable` 的准入规则**（`stable-kernel-rules.rst` 实证，五条全满足才行）：

| # | 规则 |
|---|------|
| 1 | **它或等价修复必须已经在 Linus 的树里** |
| 2 | 必须**明显正确且已测试** |
| 3 | **不得超过 100 行**（含上下文） |
| 4 | 必须遵守 `submitting-patches.rst` |
| 5 | 要么修一个**真正困扰用户的 bug**，要么只是加一个 device ID |

| 明确**不收**的 | 原文表述 |
|--------------|---------|
| 理论问题 | *"No 'This could be a problem...' type of things like a 'theoretical race condition'"*——**除非同时给出可利用性说明** |
| 琐碎修改 | *"No 'trivial' fixes without benefit for users"*（拼写、空白清理） |
| 安全补丁 | 不该只走 stable 流程，要走 `Documentation/process/security-bugs.rst` |

> **三种提交方式，第一种强烈推荐**：在**提交给主线时**就把
> `Cc: stable@vger.kernel.org` 写进签名区——补丁进主线后会被**自动**回传，
> 无需作者或维护者再做任何事。（方式 2/3 是事后请 stable 团队捡，或手工适配老版本。）

---

### 生成与发送：命令实录

```bash
# 1) 生成：单发 / 系列 / 带封面信
git format-patch -1
git format-patch origin/main -o out/
git format-patch --cover-letter -3 -o out/          # 系列：额外生成 0000-cover-letter.patch

# 2) v2 附上与 v1 的差异（reviewer 最爱看的东西）
git format-patch -3 -o out/ --subject-prefix="PATCH v2" \
    --range-diff v1~3..v1                            # 在 --- 之后插入 range-diff

# 3) 找收件人（注意 --no-rolestats，见 20.3 的坑）
scripts/get_maintainer.pl --no-rolestats --separator ', ' -f net/core/dev.c

# 4) 发送（先 --dry-run！先发给自己！）
git send-email --dry-run --to=davem@davemloft.net --cc=netdev@vger.kernel.org out/*.patch
```

> **两条铁律**：`--dry-run` 先看一遍；**先发给自己**确认补丁没被邮件客户端折行
> （呼应 [20.1](./section-20.1-社区.md) 与 `email-clients.rst`）。

---

### 迭代与耐心

| 阶段 | 该做什么 |
|------|---------|
| v2 / v3 | 标题加 `v2`；**变更说明写在 `---` 之后**；用 `--range-diff` 让 reviewer 一眼看出改了哪几行 |
| 收到 tag | 加到下一版；若改动太大要**移除并说明** |
| 被 NACK | 按意见改，或**给出技术反驳**（对事不对人，见 CoC） |
| 错过 merge window | **等下一个开发周期**（新特性在 rc 期间不受欢迎，见 20.1） |
| 长时间无回应 | 可礼貌 ping；同时检查是否发错列表/维护者 |

---

### HFT 视角：内部内核 fork 的补丁纪律

即使补丁**永远不往上游发**，内核这套纪律也值得照搬：

| 纪律 | 收益 |
|------|------|
| **一个 commit = 一个逻辑变更** | 出问题时能 `git revert` 单个变更，也能 `git bisect` 精确定位 |
| **summary 写清 what + why** | 一年后排查延迟毛刺时，`git log --oneline` 本身就是文档 |
| **`Fixes:` 追踪** | 每个本地补丁标注它修的上游 commit，rebase 到新内核时可自动判断"上游是否已吸收" |
| **保留 `Link:`** | 指向内部评审记录 / 上游讨论，形成可追溯链 |
| **backport 单独成层** | 本地补丁与上游代码分开放（或固定 rebase 顺序），升级内核时冲突面最小 |
| **小步 + 可 bisect** | 中间每个 commit 都能编译能启动——这是 `git bisect` 能工作的前提 |

<details>
<summary>自测题（点击展开）</summary>

**Q1.** git format-patch 生成的补丁格式是什么？

<details><summary>答案</summary>

补丁 = 邮件格式：Subject（[PATCH] 前缀）、From、Date、Message-Id、commit message、--- 分隔符、diffstat、diff 内容。`git format-patch -1 HEAD` 生成最近 1 个 commit 的补丁。`git send-email --to=maintainer --cc=linux-kernel@vger.kernel.org 0001-*.patch` 发送。多补丁系列用 [PATCH 1/5] ~ [PATCH 5/5] 编号。

> **补全 canonical 结构**（v6.6 `submitting-patches.rst`）：
> `---` **之上**的部分（From 行、正文、`Signed-off-by:`、`Fixes:` / `Link:` / `Closes:` 等标签）
> **会永久进 changelog**；`---` **之下**（版本变更说明、range-diff）**不进**。
> Subject 格式是 `[PATCH 001/123] subsystem: summary phrase`，summary 限 70~75 字符、
> 须同时说清 what 与 why；序列号**零填充**是为了让文本排序与数字排序一致。

</details>

**Q2.** `Fixes:` 标签的写法有什么硬性要求？加了它就等于自动进 stable 吗？

<details><summary>答案</summary>

写法要求：用 **SHA-1 前 12 位 + 单行摘要**，且**不能折行**（tag 豁免 75 列换行规则，目的是方便脚本解析）。可用 `git config pretty.fixes 'Fixes: %h ("%s")'` + `git log -1 --pretty=fixes <sha>` 自动生成。**不等于自动进 stable**——文档原文明说"Attaching a Fixes: tag does not subvert the stable kernel rules process nor the requirement to Cc: stable@vger.kernel.org"。要进 stable 必须另加 `Cc: stable@vger.kernel.org`，并且满足五条准入规则（已在 Linus 树中 / 明显正确且已测试 / ≤100 行含上下文 / 遵守 submitting-patches / 修真实 bug 或加设备 ID）。

</details>

**Q3.** `Reviewed-by:` 和 `Acked-by:` 有什么区别？`Co-developed-by:` 有什么特殊约束？

<details><summary>答案</summary>

`Reviewed-by:` 更正式，附带四条声明（技术评审已做、问题已反馈且满意、认为此刻值得合入且无已知阻碍、**不作任何担保**——它是"意见陈述"不是背书）。`Acked-by:` 不如 Reviewed-by 正式，常用于**受影响代码**的维护者表示"我不反对"，且**不代表认可整个补丁**（跨子系统补丁可以只 ack 自己那部分）。`Co-developed-by:` 的硬约束是：**每个 `Co-developed-by:` 必须紧跟该共同作者的 `Signed-off-by:`**，因为 Co-developed-by 表示共同作者身份，作者身份必须有人签字。

</details>

**Q4.** 为什么内核强调"一个补丁 = 一个逻辑变更"？这对下游团队有什么实际价值？

<details><summary>答案</summary>

对上游：reviewer 可以**逐条 NACK 系列中的某一封**，混在一起的补丁既没法部分接受，也没法单独 revert。对下游（包括内部 fork）价值更大：① 出问题时能 revert 单个逻辑变更而不连带其他改动；② `git bisect` 能定位到精确的语义变更（前提是每个 commit 都能编译能启动）；③ summary 写清 what+why 后，`git log --oneline` 本身就是可检索的设计文档；④ 用 `Fixes:` 标注本地补丁所修的上游 commit，rebase 到新内核时能自动判断"这个本地补丁是不是已被上游吸收"。

</details>

</details>
---
