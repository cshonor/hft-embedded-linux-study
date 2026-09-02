# Ch 1 §2 源码管理 (Managing the Source)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`git format-patch` / `git am` / `git send-email` 工作流）

---

## 本节讲什么

本节回答三个问题：

1. 一个补丁文件到底**长什么样**——unified diff 的每个字段是干嘛的？
2. 原书的 `diff`/`patch`/PatchSet 在现代 git 工作流里的**等价物**是什么？
3. 为什么说「可复现的内核树 = base + ordered patches + config」这个思路至今不过时？

内核协作以**补丁 (patch)** 为单位——比传整份源码**小、可审、可叠加**。理解补丁的物理形态，是后面「读代码、提交补丁」两件事的共同地基。

---

## 1. unified diff 解剖

`diff -u old.c new.c` 输出的就是补丁文件。逐字段拆开看：

```
--- a/mm/page_alloc.c  2026-01-01 00:00:00.000000000 +0000   ← 旧文件（---）
+++ b/mm/page_alloc.c  2026-01-01 00:00:00.000000000 +0000   ← 新文件（+++）
@@ -1200,7 +1200,8 @@ void __alloc_pages(...)    ← hunk 头：旧 1200 起 7 行 / 新 1200 起 8 行
         gfp_mask &= gfp_allowed_mask;        ← 上下文行（空格开头，定位用）
-        return NULL;                         ← 删除行（- 开头）
+        if (unlikely(!node_online(nid)))     ← 新增行（+ 开头）
+                return NULL;
         page = get_page_from_freelist(...);  ← 上下文行
```

| 字段 | 含义 |
|------|------|
| `---` / `+++` | 旧/新文件的路径（`a/`、`b/` 前缀是 git 的惯例） |
| `@@ -L,C +L,C @@` | **hunk 头**：旧文件从第 L 行起 C 行、新文件从第 L 行起 C 行；后面跟**所在函数名**（`-p` 开启） |
| 空格开头 | **上下文行**，两版都有，帮助 `patch` 定位 |
| `-` 开头 | 旧文件有、要删掉的行 |
| `+` 开头 | 新文件有、要加上的行 |

**关键直觉：补丁是「按行做差」的结果，不是「整文件替换」。** 所以它小、能审、能叠加——同一个函数里不同人改不同行，补丁之间互不冲突。这就是为什么内核能用「邮件发补丁」协作几十年。

```bash
diff -u old.c new.c > fix.patch      # 生成
patch -p1 < fix.patch                # 应用（-p1 剥掉 a/ b/ 前缀）
patch -p1 --dry-run < fix.patch      # 只测试能否干净打上，不改文件
patch -p1 -R < fix.patch             # 反向撤销（revert）
```

---

## 2. git 时代的等价物

原书的 `diff`/`patch` 手工流，在 git 里有更稳的对应物：

| 原书 | git 等价物 | 说明 |
|------|-----------|------|
| `diff -u` | `git diff` / `git format-patch` | `git diff` 看未提交改动；`format-patch` 把提交转成**带 commit message 的补丁** |
| `patch -p1` | `git apply` / `git am` | `apply` 只打内容不打提交历史；`am` 连提交信息一起还原 |
| PatchSet（规范文件） | `git branch` + `.config` 进版本库 | 一个分支 = 一条补丁链 |
| 手工邮件 | `git send-email` | 把 `format-patch` 产出的补丁直接发到列表 |

**`format-patch` → `am` 的完整闭环：**

```bash
git format-patch -3                     # 把最近 3 个提交转成 0001/0002/0003 三个 .patch
git format-patch -o patches/ main..HEAD # 导出 main 之后的所有提交
git am patches/*.patch                  # 在另一棵树/另一个人那里按序还原，保留 commit message + author
```

`format-patch` 产出的补丁是 **mbox 格式**：一个文件里多个邮件，每个以 `From <sha1> Mon Sep 17...` 开头，`Subject:` 是提交标题，正文是 diff。所以它**既能 `git am` 应用，也能直接 `git send-email` 发出**——补丁和邮件在此合二为一，这是理解「内核=邮件列表驱动」的关键。

---

## 3. PatchSet 的现代转世：quilt 与 patch 队列

原书的 **PatchSet** 用一份规范文件声明「基于哪版内核 + 依次打哪些补丁 + 用哪份 config」，本质是**把一条补丁链变成可复现的构建**。今天的等价物：

| 工具 | 机制 | 谁在用 |
|------|------|--------|
| **quilt** | `series` 文件列补丁顺序，`push`/`pop` 进栈出栈 | Debian 内核包维护 |
| **git 分支** | 每个补丁一个 commit，历史即补丁链 | 主流 |
| **发行版 src.rpm / deb** | base 源码 + `.patch` 队列 + config | 红帽/Debian |

```bash
quilt new fix-vmalloc.patch   # 新建一个补丁
quilt add mm/vmalloc.c        # 声明要改的文件
# ... 编辑文件 ...
quilt refresh                 # 把改动收进当前补丁
quilt series                  # 看整条补丁链的顺序
```

**核心思路不过时的部分**：「**可复现的内核树 = base 版本 + 有序补丁 + 一份 config**」。无论 git 还是 quilt，本质都是把这三个要素固化下来，让任何人能**从同一 base 重建出逐位一致的内核**。

---

## 4. 嵌入式 / HFT 关联：vendor BSP 补丁链

这条链在嵌入式里是**日常刚需**，不是理论：

```
上游 v6.6 (torvalds/linux)
  ├─ SoC 厂商补丁（树莓派 bcm2712、NXP i.MX、瑞芯微 rk...）
  │     └─ 有时几百个补丁，vendor 用 quilt/git 维护
  ├─ 板级 BSP 补丁（网卡/电源/时钟的 out-of-tree 改动）
  └─ 自己的性能补丁（锁优化、中断亲和、大页策略）
        └─ 最终出厂的 defconfig
```

**HFT 关联**：你的网关机/采集卡内核，通常就是「上游 + vendor 补丁 + 自己的延迟优化补丁」这条链。**把这条链版本化**（git 分支 + defconfig 入库）后，才能：
1. 上游升版时**逐补丁 rebase**，快速定位「哪个 vendor 补丁和 6.7 冲突了」；
2. **回归定位**——延迟突然变差时，二分这条补丁链，找出是哪一笔引入的。

这正是「可复现内核树」在 HFT 里的直接价值：**性能可复现，才能性能可调试**。

---

## 5. 衔接

- 下节 [§3 浏览代码](./section-3-浏览代码.md)：拿到可复现的树后，怎么高效读它
- [§5 提交补丁](./section-5-提交补丁.md)：`format-patch` 产出的补丁怎么发、怎么过审

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`git apply` 和 `git am` 到底差在哪？**
A：`git apply` 只把 diff **内容**打到工作区，**不产生提交**（等价于 `patch -p1`，但更严格、支持二进制）。`git am` 把 mbox 格式的补丁**连提交信息、作者、时间一起**还原成 commit。发来的补丁要保留提交链用 `am`，只想看改动效果用 `apply`。

**Q2：hunk 头 `@@ -1200,7 +1200,8 @@` 里的数字错了会怎样？**
A：`patch` 靠这些数字**定位**，不是硬性要求——有上下文行兜底，稍有偏移也能**模糊匹配**打上。数字严重对不上时会输出 `Hunk #1 FAILED` 并生成 `.rej` 文件，让开发者手工处理冲突。这就是为什么补丁要打在**干净的、版本一致的 base** 上。

**Q3：为什么补丁要 `-p1` 而不是 `-p0`？**
A：git 的补丁路径带 `a/`、`b/` 前缀（`a/mm/page_alloc.c`）。`-p1` 剥掉第一层目录（`a/`），得到真实路径 `mm/page_alloc.c`；`-p0` 不剥，会把文件打到 `a/mm/...` 这种不存在的路径。手工 `diff -u` 生成的补丁没有 `a/` 前缀，用 `-p0`。

**Q4：`git format-patch` 和 `git diff > x.patch` 有什么本质区别？**
A：`git diff > x.patch` 只是**裸 diff**，没有提交标题、作者、commit message，别人 `am` 时不知道「这是谁、为什么改」。`format-patch` 把**整个提交（含 message + author + 时间）**序列化成邮件格式，是发给上游评审的**正确形态**。

**Q5：vendor 的几百个补丁，为什么不能直接 rebase 到新内核？**
A：vendor 补丁经常**依赖特定 base 版本的内部函数/结构体**，上游一改就冲突。所以嵌入式升级是「**逐补丁 rebase**」——一个个 cherry-pick 上去，冲突的重新适配，而不是整条链硬 rebase。这也是为什么「base + ordered patches」要**显式记录顺序和 base 版本**。

</details>

---
