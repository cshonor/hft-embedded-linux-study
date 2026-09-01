## ④ 提交错误报告 · Bug Reports

| 必备信息 | |
|----------|--|
| **症状** | 发生了什么 |
| **系统输出** | dmesg、配置 |
| **完全解码的 Oops** | 若有（Ch 18 · `kallsyms`） |
| **稳定复现步骤** | |
| **硬件说明** | 架构、设备 |

| 发往 | **`MAINTAINERS` 中相关维护者** + **抄送 LKML** |

| 原则 | **能复现 > 猜测** · **完整 Oops > 截图一行** |

→ v6.6 现行文档：**`Documentation/admin-guide/reporting-issues.rst`**（比本书详细得多）

---

### 必带清单（v6.6 实证）

`reporting-issues.rst` 的 *"Things each report should mention"* 列了四条**必须给**的：

| 必带 | 怎么拿 |
|------|--------|
| **`cat /proc/version`** | 内核版本号**和构建它的编译器**（编译器不同，行为可能不同） |
| 发行版 | `hostnamectl \| grep "Operating System"` |
| **架构** | `uname -mi` |
| **bisect 结果**（若是回归） |  culprit commit 的 **subject + commit-id** |

还有两样**建议提供但别贴进邮件正文**（太大）：

| 建议提供 | 注意 |
|---------|------|
| **`.config`** | 内核构建配置 |
| **`dmesg` 输出** | 必须以 `Linux version 5.8-1 (...) (gcc ...) #1 SMP ...` 这样的行**开头**；缺失说明早期日志已被丢弃，改用 `journalctl -b 0 -k`，或重启复现后立刻 `dmesg` |

> **大文件的正确处理方式**（原文）：上传到公共位置并在报告里给链接（**选能存很多年的地方**
> —— 原文理由是"五年十年后有人改这段修复代码时可能还要看"）；
> 或者说一句"稍后单独回复附上"，**然后记得真的发**。
> 用邮件报告时**不要做附件**——邮件太大会直接被跳过（呼应 [20.1](./section-20.1-社区.md)）。

---

### 先排 taint：一条能省几小时的检查

内核会给**自己**打 taint 标记，表示"发生过某件事，之后任何诡异现象都可能是次生灾害"。
报告之前**先确认它不是 tainted**：

```bash
cat /proc/sys/kernel/tainted     # 返回 0 = 干净
```

| 检查途径 | 判据 |
|---------|------|
| 运行时 | `/proc/sys/kernel/tainted` == `0` |
| Oops / panic 现场 | 找以 `CPU:` 开头的那一行，结尾是 **`Not tainted`** 才是干净的；出现 `Tainted:` + 字母即为已污染 |

> **最典型的一条污染原因**（文档明写）：**发生过一次 Oops 之后，内核会自我 taint**，
> 因为内核自己知道从那刻起可能行为异常。
> 所以看到 `Oops: 0000 [#1] SMP` 之后的所有现象——哪怕看起来毫无关系——
> **都可能是第一个 Oops 的次生错误**。
> 正确做法：先消灭首个 Oops（重启、或改配置重启），再复现你真正想报的问题。
> 完整原因列表见 `Documentation/admin-guide/tainted-kernels.rst`。

---

### 回归：内核社区的第一法则

`reporting-regressions.rst` 开篇原文：

> *"**We don't cause regressions**" is the first rule of Linux kernel development;
> Linux founder and lead developer Linus Torvalds established it himself and
> ensures it's obeyed.*

判定标准（原文）：**在配置相似的前提下，某个应用或用例在旧内核上正常、在新内核上变差或不能用了**。
注意"配置相似"是硬条件——拿一个裁剪过的旧配置和一个全功能新配置比，不算回归。

**报告回归的三件事**（文档 TL;DR）：

| # | 做什么 | 具体形式 |
|---|--------|---------|
| 1 | 主题以 **`[REGRESSION]`** 开头 | `[REGRESSION] <bisect 出的 culprit 标题>` |
| 2 | **抄送回归专用列表** | `regressions@lists.linux.dev` |
| 3 | （推荐）让 **regzbot** 跟踪 | 正文里写一行 `#regzbot introduced: v5.13..v5.14-rc1`；若已 bisect 到 commit 就写 commit-id |

> **为什么第 3 条值得做**：regzbot 会把你的报告纳入周报和
> "未解决回归清单"，**Linus 在决定"继续开发还是就此发版"时会看这份清单**。
> 不写这行也能被人工登记，但文档直言——回归跟踪员只有一个人，
> *"just one human which sometimes has to rest"*，靠人工**必然延迟**。

---

### 完整流程：八步

```
① 确认用的是上游内核（或至少先在上游版上复现）
       │  发行版魔改过的内核，维护者一般不受理
       ▼
② 搜已有报告（lore 归档 + regressions 列表归档 + regzbot web 界面）
       ▼
③ 查 taint（见上）—— 不干净就先处理
       ▼
④ 把复现步骤最小化，并能「在新启动的系统上独立复现」
       │  多个问题 → 分开报，除非强耦合
       ▼
⑤ 判断是不是回归 → 是则走回归通道（[REGRESSION] + regzbot）
       ▼
⑥ 找对维护者（scripts/get_maintainer.pl，见 [20.3](./section-20.3-指挥链.md)）
       ▼
⑦ 写报告：倒着写 —— 先写细节，最后写开头
       │  开头 = 一句话摘要 + 一段概述；很多人只看这部分决定要不要往下读
       ▼
⑧ 发出之后的义务：回应提问、提供补充数据、测试维护者给的补丁
```

> **只对你的报告做一件事的优化**：把**开头**写好。文档原话：
> *"a lot of people will only read this before they decide if reading the rest
> is time well spent."*

---

### HFT 视角：延迟抖动这类「难复现」问题怎么报

内核社区能受理的不是"我的延迟变高了"，而是**可量测、可对比、可 bisect**的证据。

| 该给什么 | 说明 |
|---------|------|
| **对照的分位数** | 旧内核 vs 新内核的 p50 / p99 / p99.9，**同一负载模型、至少同样时长** |
| **量测方法本身** | cyclictest / `perf sched` / ftrace / bpftrace 脚本（见 [06.7 eBPF 观测](../../../06.7-bpf-observability/)）；**测量方法不可信，数据就没意义** |
| **拓扑与配置** | CPU 绑核、NUMA 节点、时钟源（`clocksource`）、isolcpus / nohz_full、是否有 RT 补丁 |
| **负载模型** | 是行情回放还是压测工具？包速率、连接数、是否有 GC/日志抖动源 |
| **bisect 结果** | 这是最有分量的一项：**把"延迟变差"收敛到一个 commit**，否则几乎不可能被受理 |

> **关键判断**：延迟问题如果**不能 bisect**，在社区里基本等于"无法处理"。
> 所以实操顺序是：先做可重复的基准（能稳定复现 p99 差异），
> 再 bisect，**最后**才写报告。这一步走完，你的报告才有被认真对待的资格。

> **另一个现实路径**：HFT 常见的延迟毛刺往往出在**配置与拓扑**
> （绑核、中断亲和、NUMA、透明大页、C-states）而非内核 bug。
> 报之前先确认旧内核和新内核跑的是**同一套配置**——
> 否则你报的"回归"，文档里那句"配置相似"的前提就不成立了。

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 一个好的内核 bug 报告应该包含什么？

<details><summary>答案</summary>

1) 内核版本（`uname -r`）；2) .config（配置）；3) 复现步骤（最小化）；4) dmesg/oops 日志；5) 硬件信息（CPU/内存/设备）；6) 是否能 bisect；7) 已尝试的排查。HFT 系统的内核 bug 报告还应包含：交易负载特征、时序信息（延迟突增时间点）、NUMA 配置、实时补丁版本。

> **按 v6.6 补充四条必带项**（`reporting-issues.rst`）：
> `cat /proc/version`（含**编译器版本**）、发行版（`hostnamectl \| grep "Operating System"`）、
> 架构（`uname -mi`）、以及**回归场景下 bisect 出的 culprit subject + commit-id**。
> 另外：`.config` 和 `dmesg` 建议提供但**不要贴进邮件正文**（太大），
> 应上传公共位置给链接；`dmesg` 还必须确认以 `Linux version ...` 行开头，否则早期日志已丢。

</details>

**Q2.** 报告前为什么要先查 `/proc/sys/kernel/tainted`？

<details><summary>答案</summary>

因为内核在"发生过可能导致后续错误的事情"时会给自己打 taint 标记，之后出现的**任何**诡异现象都可能是次生灾害，不是真正的 bug。返回 `0` 才是干净的；Oops/panic 现场则看以 `CPU:` 开头的那行结尾是否为 `Not tainted`。最典型的污染原因是**发生过一次 Oops 之后内核自我 taint**——从那刻起的所有现象（哪怕看起来无关）都可能是第一个 Oops 的次生错误。正确做法是先消灭首个 Oops（重启或改配置重启），再复现你想报的问题。这一步能省掉几小时无效排查。

</details>

**Q3.** 什么叫「回归」，报告它和普通 bug 有什么不同？

<details><summary>答案</summary>

判定标准：**配置相似的前提下**，某个应用或用例在旧内核上正常、在新内核上变差或不能用。"配置相似"是硬条件。"We don't cause regressions" 是内核开发的第一法则。不同之处有三：① 主题必须以 `[REGRESSION]` 开头（若 bisect 成功，用 culprit commit 的标题作为主题第二部分）；② 抄送 `regressions@lists.linux.dev`；③ 推荐在正文写一行 `#regzbot introduced: v5.13..v5.14-rc1`（或 commit-id）让 regzbot 跟踪——这会让你的问题进入周报和未解决回归清单，而 Linus 在决定"继续开发还是发版"时会看这份清单。

</details>

**Q4.** HFT 场景下「升级内核后 p99 延迟变差」该怎么报告才可能被受理？

<details><summary>答案</summary>

核心是把"感觉变慢"变成"可量测、可对比、可 bisect"的证据：① 先做可重复的基准，能在**同一负载模型**下稳定复现旧/新内核的 p50/p99/p99.9 差异；② **bisect 到具体 commit** —— 这是最有分量的一项，不能 bisect 的延迟问题在社区基本等于无法处理；③ 给出量测方法本身（cyclictest / ftrace / bpftrace 脚本），因为测量方法不可信则数据无意义；④ 给出拓扑与配置：绑核、NUMA、时钟源、isolcpus/nohz_full、是否 RT；⑤ 确认新旧内核跑的是同一套配置，否则"配置相似"这个回归前提不成立，你遇到的可能只是配置差异而非内核 bug；⑥ 走回归通道：`[REGRESSION]` 主题 + 抄送 regressions 列表 + regzbot 命令。

</details>

</details>
---
