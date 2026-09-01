## ③ 指挥链 · Chain of Command

| 文件 | **`MAINTAINERS`**（源码树根目录） |
|------|-----------------------------------|
| 内容 | 各 **驱动/子系统维护者** 名单与范围 |

| 顶层 | **Linus** — **主线（mainline）** 树最终维护者 |

```
你的网卡驱动补丁
    ▼
查 MAINTAINERS → netdev 维护者
    ▼
发邮件（非直接乱投 Linus，除非小全局改动）
```

---

### `MAINTAINERS` 字段字典（v6.6 实证）

文件开头有一节 **"Descriptions of section entries and preferred order"**，
逐字段说明如下（照抄次序，写条目时也要按这个顺序）：

| 字段 | 含义 | 备注 |
|------|------|------|
| `M:` | ***Mail* patches to** —— 维护者 | 补丁的**主要收件人** |
| `R:` | Designated ***Reviewer*** —— 指定评审者 | **必须抄送**（"These reviewers should be CCed on patches"） |
| `L:` | ***Mailing list*** —— 相关邮件列表 | 抄送 |
| `S:` | ***Status*** —— 维护状态 | 五种取值，见下 |
| `W:` | ***Web-page*** —— 状态/信息页 | |
| `Q:` | ***Patchwork*** —— 补丁跟踪站点 | 如 netdev 的 `patchwork.kernel.org/project/netdevbpf/list/` |
| `B:` | URI for where to file ***bugs*** | 网页或 `mailto:` |
| `C:` | URI for ***chat*** | 如 `irc://server/channel` |
| `P:` | Subsystem **Profile** 文档 | 该子系统的投稿细则（见 `Documentation/maintainer/maintainer-entry-profile.rst`） |
| `T:` | ***SCM* tree** 类型与位置 | 类型：`git` / `hg` / `quilt` / `stgit` / `topgit` |
| `F:` | ***Files*** —— 文件/目录通配 | `drivers/net/` 含子目录；`drivers/net/*` 不含；`*/net/*` 任意顶层目录下的 net |
| `X:` | ***Excluded*** files —— 排除项 | **判定顺序先于 `F:`**（先剔除再匹配） |
| `N:` | Files ***Regex*** 正则匹配 | 与 `F:` 的**语义不同**，见下 |
| `K:` | ***Content regex*** —— 按**补丁/文件内容**匹配 | 如 `K: \b(printk\|pr_(info\|err))\b` |

---

### `S:` 状态：五种取值决定你的补丁有没有人管

| 状态 | 含义 | 对你的意义 |
|------|------|-----------|
| **Supported** | 有人**拿工资**专门管 | 响应最快 |
| **Maintained** | 有人实际在管 | 正常路径 |
| **Odd Fixes** | 有维护者，但**只投得起零散补丁**，没时间做更多 | 复杂改动可能长期没人接 |
| **Orphan** | **没有维护者**（原文：*"but maybe you could take the role as you write your new code"*） | 你改这块，基本等于你自己接手 |
| **Obsolete** | 旧代码，已被更好的系统取代 | **你不该再用它** |

> **彩蛋**：文件里有一条 `THE REST`：
> ```
> THE REST
> M:	Linus Torvalds <torvalds@linux-foundation.org>
> L:	linux-kernel@vger.kernel.org
> S:	Buried alive in reporters      ← 状态字段写的是这个
> T:	git git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
> F:	*
> F:	*/
> ```
> `F: *` / `F: */` 表示**兜底匹配全树**——没有更精确条目的文件，最终落到 Linus 名下。
> 这就是为什么"找不到维护者时发给 LKML"是有效的：**兜底条目真的存在**。

---

### `F:` 和 `N:` 不是一回事（影响 `get_maintainer` 行为）

`MAINTAINERS` 头部原文明确写了这个差别，非常容易踩：

| 字段 | 命中后 `get_maintainer` 的行为 |
|------|------------------------------|
| **`F:`**（通配） | **不查 git log 历史**，只按条目里的人通知 |
| **`N:`**（正则） | **会用 git log 历史**，额外通知那些提交带签名的人 |
| **`X:`** | 排除项，**先于 `F:` 判定**（先剔除，再匹配） |

```makefile
F:	net/
X:	net/ipv6/
# → 匹配 net/ 下除 net/ipv6/ 之外的全部
```

---

### 两个真实条目：网络与 BPF（v6.6 实证）

```
NETWORKING [GENERAL]                    BPF [GENERAL] (Safe Dynamic Programs and Tools)
M:	"David S. Miller" <...>         M:	Alexei Starovoitov <...>
M:	Eric Dumazet <...>              M:	Daniel Borkmann <...>
M:	Jakub Kicinski <...>            M:	Andrii Nakryiko <...>
M:	Paolo Abeni <...>               R:	Martin KaFai Lau <...>   ← 7 位 reviewer
L:	netdev@vger.kernel.org          L:	bpf@vger.kernel.org
S:	Maintained                      S:	Supported                ← 拿工资管
Q:	https://patchwork.kernel.org/project/netdevbpf/list/
B:	mailto:netdev@vger.kernel.org   Q:	...netdevbpf/list/?delegate=121173
T:	git .../netdev/net.git          T:	git .../bpf/bpf.git
T:	git .../netdev/net-next.git     T:	git .../bpf/bpf-next.git
F:	net/  F:	include/net/  ...        F:	kernel/bpf/  F:	include/uapi/linux/bpf.h ...
```

> 注意 `T:` 有两棵树：**`net` / `net-next`** 与 **`bpf` / `bpf-next`**。
> 命名带 `-next` 的那棵是**收集新特性**的树（只在合并窗口期间汇入主线），
> 不带 `-next` 的那棵收 **fixes**。发对树、发对时机，是能被合入的前提。

---

### 从你到 Linus：六层

```
① 贡献者（你）
      │  To: M: 维护者   Cc: R: 评审者 + L: 列表 + 相关开发者
      ▼
② reviewer —— 技术评审，给 Reviewed-by / NACK
      │
      ▼
③ 子系统维护者 —— 决定收不收，收进自己的树（T: 字段）
      │  Acked-by / Signed-off-by
      ▼
④ linux-next —— 所有子系统树的集成构建/冒烟测试场地
      │  每天自动构建，跨树冲突在这里先炸
      ▼
⑤ merge window（约 2 周）—— Linus 从各子系统树 pull
      ▼
⑥ mainline（vX.Y-rc1 → vX.Y）→ stable 团队接手
```

> `Documentation/process/5.Posting` 对"能不能直接发给 Linus"的回答很明确：
> *"While it is possible to send patches directly to Linus Torvalds and have
> him merge them, things are not normally done that way. Linus is busy...
> If there is no obvious maintainer, Andrew Morton is often the patch target
> of last resort."*
> —— **找不到维护者时的兜底收件人是 Andrew Morton**，不是 Linus。

---

### 用 `scripts/get_maintainer.pl` 自动找人

手写收件人列表既慢又容易漏，正确做法是一行命令：

```bash
# 基本用法：按补丁内容找人
scripts/get_maintainer.pl 0001-net-fix-race-in-foo.patch

# 按文件找人
scripts/get_maintainer.pl -f net/core/dev.c

# 只要能直接喂给 git send-email 的地址（去掉角色统计）
scripts/get_maintainer.pl --no-rolestats --separator ', ' -f drivers/net/foo.c

# 连 git 历史里改过这几行的人一起通知（谨慎，会拉进很多人）
scripts/get_maintainer.pl --git --git-min-percent=10 -f net/core/dev.c
```

**v6.6 的默认选项串**（`--help` 里的 Default options）：

```
[--email --tree --nogit --git-fallback --m --r --n --l --multiline
 --pattern-depth=0 --remove-duplicates --rolestats]
```

> ⚠️ **一个实测会踩的坑**（`--help` 里明写）：
> `--roles` / `--rolestats` 会在地址**后面追加**角色与统计文本，
> 因此**不能直接喂给 `git send-email --cc-cmd`** 这类只接受
> `["name"] <email>` 的自动化工具。要喂给 `send-email` 就必须加 `--no-rolestats`。

---

### HFT 视角：你真正该盯的上游是这几个

| 关注点 | 上游 | v6.6 实证的负责人 / 列表 |
|--------|------|------------------------|
| 收包路径、网卡驱动、TSO/GRO | `netdev@vger.kernel.org` | David S. Miller / Eric Dumazet / Jakub Kicinski / Paolo Abeni（`S: Maintained`） |
| eBPF、verifier、观测能力 | `bpf@vger.kernel.org` | Alexei Starovoitov / Daniel Borkmann / Andrii Nakryiko（`S: Supported`） |
| **实时调度策略 `SCHED_FIFO` / `SCHED_RR`** | 调度器条目 | **Steven Rostedt** 是这个子方向的 reviewer（见 `SCHEDULER` 条目） |
| 普通调度（CFS/调度延迟） | 调度器条目 | Vincent Guittot（`SCHED_NORMAL`）、Peter Zijlstra、Ingo Molnar |
| RT 抢占的运行问题 | `linux-rt-users@vger.kernel.org` | 用户与开发者混合列表 |

> 实用建议：**把这几条列表的 lore 归档加进订阅**，比读二手博客快半拍。
> 调度与网络的行为变更，永远先在列表上吵完，才进发行版内核——
> 而 HFT 系统的延迟毛刺，经常就来自某个"看起来无害"的上游改动
> （所以 [20.4](./section-20.4-提交错误报告.md) 里回归通道很重要）。

<details>
<summary>自测题（点击展开）</summary>

**Q1.** MAINTAINERS 文件的作用？补丁如何从贡献者到 Linus？

<details><summary>答案</summary>

MAINTAINERS 记录每个子系统的维护者（名字/邮箱/状态/SCM/审计状态）。补丁路径：贡献者 → 子系统维护者 → 子系统树 → linux-next（集成测试）→ Linus（mainline merge window）。层层 review 保证质量。HFT 公司如果有内核修改，需要找对子系统维护者提交，不能直接发 Linus。

</details>

**Q2.** MAINTAINERS 里 `F:` 和 `N:` 都表示"匹配文件"，二者有什么实质区别？

<details><summary>答案</summary>

区别在于 `get_maintainer.pl` 的行为（MAINTAINERS 头部原文）：命中 **`F:`（通配路径）** 时，脚本**不查 git log 历史**，只通知条目里列出的人；命中 **`N:`（正则路径）** 时，脚本**会用 git 历史**，额外通知那些在相关提交上带签名的人。此外 `X:`（排除）的判定**优先于** `F:`——先剔除再匹配。所以给一个子系统写条目时，能用精确 `F:` 路径就别用 `N:`，否则收件人会随历史波动、范围不可控。

</details>

**Q3.** 一个子系统的 `S:` 字段标着 `Odd Fixes` 或 `Orphan`，意味着什么？

<details><summary>答案</summary>

`Odd Fixes` = 有维护者但**只投得起零散补丁**、没时间做更多，复杂改动可能长期没人接；`Orphan` = **没有维护者**，文件原文还补了一句"*but maybe you could take the role as you write your new code*"——你要改这块，基本上等于自己接手。五种状态是 Supported（有人拿工资管）/ Maintained（有人实际在管）/ Odd Fixes / Orphan / Obsolete（旧代码，已被更好的系统取代，不该再用）。**写驱动前先看 S: 字段**，能预判你的补丁会不会石沉大海。

</details>

**Q4.** 为什么 `get_maintainer.pl` 的输出有时不能直接喂给 `git send-email --cc-cmd`？

<details><summary>答案</summary>

因为默认选项里带 `--rolestats`，它会在每个地址**后面追加**角色与提交统计文本（如 `maintainer:NETWORKING [GENERAL] (12/45, 27%)`），而 `--cc-cmd` 这类自动化工具只接受 `["name"] <email address>` 格式。脚本 `--help` 里明写了这一点。解决办法是加 `--no-rolestats`（常配合 `--separator ', '`），只输出纯地址列表。

</details>

</details>
---
