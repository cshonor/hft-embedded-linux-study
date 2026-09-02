# Ch 1 §5 提交补丁 (Submitting Patches)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`scripts/checkpatch.pl` / `scripts/get_maintainer.pl` / `Documentation/process/`）

---

## 本节讲什么

本节回答三个问题：

1. 改完内核代码后，**提交前要过哪些「门」**——风格、检查工具、changelog 各是什么？
2. `Signed-off-by` 到底签的是什么，为什么它比「同意合并」更严肃？
3. 一个补丁从本地到合入 Linus 主线，**完整生命周期**长什么样？

原书的 `Documentation/CodingStyle` / `Documentation/SubmittingPatches` 在 v6.6 已移到 `Documentation/process/` 下并改成 `.rst`，但**规则精神没变**。

---

## 1. 风格门：Coding Style + checkpatch

内核风格的核心规则（`Documentation/process/coding-style.rst`）：

| 规则 | 内容 | 为什么 |
|------|------|--------|
| **缩进 = Tab（8 列）** | 用 Tab 不用空格；Tab 宽 = 8 | 8 列缩进**逼你控制嵌套深度**——超过 3 层就该拆函数了 |
| **行长 ≤ 80 列** | 尽量 80，可略超但要「拆得合理」 | 终端审查友好 |
| **大括号** | 函数的大括号**独占一行**；`if/for/switch` 的跟在语句后 | K&R 风格，且函数独占一行是「函数的特权」 |
| **命名** | 局部变量短名（`i`、`tmp`），全局函数**描述性** | 全局名要跨文件可读 |
| **少 typedef** | 尽量不造新类型别名，直接用 `struct` | `typedef` 掩盖真实类型，坏处 > 好处 |
| **指针 `*` 靠变量** | `char *s`，不是 `char* s` | 与声明语义对齐 |

**但没人靠肉眼背规则**——内核有自动检查器 **`checkpatch.pl`**，这是提交前的**第一道硬门**：

```bash
scripts/checkpatch.pl --strict 0001-my-fix.patch   # 检查补丁（会报 warning/error）
scripts/checkpatch.pl --strict -f mm/vmalloc.c       # 检查整个文件
scripts/checkpatch.pl --types=SPACING ...            # 只看某类问题
scripts/checkpatch.pl --ignore=COMMIT_LOG_LONG_LINE ... # 忽略某类
```

`checkpatch` 报的是**风格 + 常见 bug 模式**（如 `malloc` 返回值没判空、`//` 注释、行尾空格）。**error 级必须清零，warning 要逐条看**——它也会误报，但每个 warning 你都得能说清「为什么可接受」。这条纪律正是内核「宁可慢、不可乱」文化在工具层的体现。

---

## 2. 提交门：changelog 与 `Signed-off-by`

补丁的 **changelog（提交说明）** 是评审者第一眼看到的东西，格式有硬约定：

```
mm: fix use-after-free in __alloc_pages fast path        ← 标题：subsystem: 一句话（祈使句，≤75 字符）

<空行>
正文：为什么改、改了什么、怎么验证。正文里要写清：
  - 触发 bug 的条件 / 观察到的现象
  - 根因（root cause）
  - 修复思路 + 关键权衡
  - Fixes: <12-char sha> ("subject")   ← 修 bug 必须带
  - Reported-by: / Suggested-by: / Tested-by: ...

<空行>
Signed-off-by: Your Name <you@example.org>               ← 见下方 DCO

---
（`---` 分隔线之后：不进入 changelog 的备注，如"v2 相对 v1 改了什么"）
```

**`Signed-off-by`（SoB）签的到底是什么？** 它不是「我同意合并」，而是对 **Developer's Certificate of Origin (DCO) 1.1** 的声明——大意为：

> 我证明：我有权提交此改动；它基于的代码要么是我原创、要么来源合规；我知晓它会作为开源发布。

SoB 的**传递链**记录了补丁的「路径」：作者 SoB → 子系统维护者 SoB → Linus SoB。**每个实际「经手」的人都要加一行**，`git commit -s` 自动加。这比「同意」严肃——它是在对**知识产权来源**作担保，出了版权纠纷靠这条链追责。

---

## 3. 发给谁：get_maintainer.pl

「发对邮件列表」是最容易翻车的一步。手动猜 `linux-mm`、`linux-kernel` 会漏人，内核提供 **`get_maintainer.pl`** 从 `MAINTAINERS` 文件自动解析：

```bash
scripts/get_maintainer.pl 0001-my-fix.patch          # 针对补丁，列出应 Cc 的人/列表
scripts/get_maintainer.pl -f mm/vmalloc.c            # 针对文件
scripts/get_maintainer.pl --nogit -f mm/page_alloc.c # 只看 MAINTAINERS，不看 git 历史
scripts/get_maintainer.pl --subsystem --scm -f mm/    # 加子系统名 + 所属 git 树
```

`MAINTAINERS` 文件按**子系统**组织，每条含：维护者（`M:`）、邮件列表（`L:`）、相关文件（`F:`）、git 树（`T:`）。`get_maintainer.pl` 会把**改动的文件**匹配到对应子系统，输出一长串 `To:` / `Cc:` 地址。**VM 补丁 → `linux-mm` 列表 + Andrew Morton（`-mm` 树的守护者）**，这是 mm 补丁的标准去处。

---

## 4. 补丁的完整生命周期

```
本地改代码
  └─ checkpatch.pl 清 error、消 warning
       └─ git commit -s（写好 changelog + SoB）
            └─ git format-patch -1 → 0001-xxx.patch
                 └─ get_maintainer.pl 拿收件人
                      └─ git send-email --to=... --cc=... 0001-xxx.patch
                           └─ 列表/维护者评审（来回改 v2/v3/v4）
                                └─ maintainer 收进自己的 git 树（linux-mm → -mm → Linus）
                                     └─ 合入 mainline，出现在下一个 merge window / -rc
```

**关键文化认知**：

| 原书时代的直觉 | 现代（v6.6）的修正 |
|----------------|-------------------|
| 补丁直接发邮件 | 仍是主流，但**用 `git send-email`** 而非手工附件 |
| 只有邮件列表 | 部分子系统有 GitLab/GitHub 镜像，但 **mm 传统仍是列表 + `-mm` 树** |
| 发一次就等合并 | **迭代是常态**——大补丁发 v2/v3/v4 很常见，`---` 分隔线后写版本变更 |
| Kernel Traffic 看摘要 | 用 **lore.kernel.org** 归档 + **patchwork** 跟踪补丁状态 |

**对 HFT / 嵌入式**：你大概率**不直接给上游发补丁**，而是改 vendor 树、发内部评审。但 `checkpatch` + changelog + SoB 这套纪律**照样适用**——它保证你的内核改动**可追溯、可审查、可回归**，这恰是「性能可复现」的前提（呼应 §2 的 vendor 补丁链）。

---

## 5. 本章带走的三句话

1. **工具：** `git format-patch`/`git am` 管理补丁 + Elixir/clangd 交叉引用 + 必要时 call graph。  
2. **读 `mm/`：** 别从 arch init 硬啃；**OOM → vmalloc → page_alloc → VMA** 由简入深建立地图。  
3. **改内核：** 先 `checkpatch` + changelog + `Signed-off-by`，用 `get_maintainer.pl` 发到**对的列表**。

---

## 6. 衔接

- 本章到此结束，正文从 [Ch2 描述物理内存](../../chapter-02-describing-physical-memory/) 开始
- 工具链回顾：[§1 入门指南](./section-1-入门指南.md)（编译）· [§2 源码管理](./section-2-源码管理.md)（补丁）· [§3 浏览代码](./section-3-浏览代码.md)（读码）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`checkpatch.pl` 报的 `error` 和 `warning` 有什么区别，怎么对待？**
A：`error` 是**硬规则**（如缺失空格、行尾空白、明显 bug 模式），提交前**必须清零**；`warning` 是**风格建议**，可能误报，但要**逐条判断**——能改就改，改不了要在 changelog 或回复里说清理由。内核社区对「无视 checkpatch 结果」零容忍。

**Q2：`Signed-off-by` 和 `Acked-by` 有什么本质区别？**
A：`Signed-off-by` 是 **DCO 声明**——签署者保证「有权提交、来源合规」，是**法律/归属**层面的担保，每个经手人都要加。`Acked-by` 是**技术认可**——维护者说「我看了，同意这个改动的方向」，是**评审**层面的记录。SoB 不能替代 review，Ack 不能替代 SoB。

**Q3：changelog 里的 `---` 分隔线是干嘛的？**
A：`git format-patch`/`send-email` 时，`---` 之前的进入**正式 commit message**，之后的内容**只出现在邮件正文里、不进入 git 历史**。所以「v2 相对 v1 改了什么」「这个补丁依赖哪个前置补丁」这类**评审辅助信息**写 `---` 之后，合入后不会污染历史。

**Q4：`get_maintainer.pl` 输出一堆地址，怎么确定「发到哪个列表」？**
A：`get_maintainer.pl` 从 `MAINTAINERS` 的 `F:`（文件匹配）找到你改动所属子系统，输出它的 `M:`（维护者）和 `L:`（列表）。**列表用 `--to`，维护者用 `--cc`**。mm 补丁的惯例是发 `linux-mm@kvack.org` + Cc `akpm`（Andrew Morton，`-mm` 树维护者）和 `linux-kernel`。

**Q5：为什么 mm 补丁走「-mm 树」而不是直接进 Linus 的树？**
A：Linus 的 `torvalds/linux` 是最终汇合点，但**每个子系统有自己的中间树**做缓冲和集中评审。mm 的中间树是 Andrew Morton 维护的 `-mm`（现在也叫 `mm` 树），补丁先在这里集成、测试、再在 merge window 批量进 Linus 树。这是「分级维护」——子系统维护者（lieutenant）先审，再推荐给 Linus。

</details>

---
