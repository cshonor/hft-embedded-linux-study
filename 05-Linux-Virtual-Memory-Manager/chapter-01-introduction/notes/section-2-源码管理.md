# Ch 1 §2 源码管理 (Managing the Source)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 2. 源码管理 (Managing the Source)

内核协作以 **补丁 (patch)** 为主——比传整份源码 **小、可审、可叠加**。

### `diff` / `patch`

| 工具 | 作用 |
|------|------|
| **`diff`** | 生成 **unified diff** 格式的差异文件 |
| **`patch`** | 把 diff **应用到** 源码树 |

日常习惯：`diff -u old new > fix.patch`，在干净树里 `patch -p1 < fix.patch`。

### PatchSet（书中工具 · 概念仍有用）

**PatchSet**：用一份 **规范文件** 声明——基于 **哪一版内核**、**依次打哪些补丁**、用 **哪份 `.config`**，减少手工 `patch` 链。

> 今天等价物：**git** 分支 / `git am` 系列、`quilt`、发行版 **patch 队列**。本书工具可能过时，**「可复现的内核树 = base + ordered patches + config」** 思路不过时。

---
