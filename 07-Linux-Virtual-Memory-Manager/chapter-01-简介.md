# Ch 1 简介 · Introduction

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**

本章 **不深入 VM 技术细节**，而是 **内核开发与源码阅读指南**：怎么拿源码、打补丁、浏览代码、按什么顺序读 `mm/`，以及怎么向社区提交改动。

---

## 本章在全书中的位置

| | 内容 |
|---|------|
| **本章** | 工具链 + 阅读方法论 + 社区流程 |
| **Ch 2 起** | 物理内存、页表、地址空间、slab、回收等 **VM 本体** |
| **附录 A–M** | 与正文对应的 **Code Commentary**（按子系统拆的源码导读） |

> **HFT 读法：** 不必死记工具名；带走两样东西——**(1) 作者推荐的 `mm/` 阅读顺序**；(2) 补丁 / 邮件列表文化。技术细节从 [Ch 2](./chapter-02-描述物理内存.md) 精读。

---

## 1. 入门指南 (Getting Started)

### 配置与编译

拿到内核源码后的典型第一步：

```bash
make config          # 交互式回答 Kconfig 选项（旧书时代常用）
make bzImage         # 编译可引导内核镜像
make modules         # 编译可加载模块
```

现代树更多用 `make menuconfig` / `make olddefconfig`；**本书成书较早**，命令名以原书为准，**流程不变**：**先配置 → 再编内核 + 模块**。

### 信息获取渠道

| 资源 | 用途 |
|------|------|
| 源码顶层 **`README`** | 版本说明、最简编译提示 |
| **`Documentation/`** | 官方文档树（含 `CodingStyle`、`SubmittingPatches`） |
| [LWN.net](https://lwn.net/) **Kernel Page** | 内核动向、深度文章 |
| **Kernelnewbies** | 新手向 FAQ、术语 |
| **LKML**（Linux Kernel Mailing List） | 补丁讨论主战场 |
| **Kernel Traffic**（及类似摘要站） | 邮件列表摘要，降低订阅全量 LKML 的成本 |

---

## 2. 源码管理 (Managing the Source)

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

## 3. 浏览代码 (Browsing the Code)

内核函数常 **跨文件、跨 `arch/`**，纯文本搜索容易迷路。原书推荐：

| 工具 | 做什么 | 今日常用替代 |
|------|--------|--------------|
| **LXR** (Linux Cross Referencing) | Web 上浏览源码；标识符 **超链接** 到定义/引用 | [elixir.bootlin.com](https://elixir.bootlin.com/linux/latest/source) · IDE **LSP/clangd** · `cscope`/`ctags` |
| **CodeViz**（作者为写书开发） | 生成 **函数调用图 (call graph)**，一眼看子系统结构 | **Doxygen Graph** · **CodeCompass** · 手工 `grep` + 画图 |

**第一次读 `mm/`：** 先能在浏览器里 **点函数名跳转**，比硬啃 `vim` 省一半时间。

---

## 4. 阅读代码的策略 (Reading the Code)

### 为什么不从「初始化代码」读起

很多老手建议从 **boot / init** 读，但作者认为 **对 VM 不优**——初始化 **高度依赖架构**（`arch/x86/`、`setup_arch`…），容易陷进硬件细节。

### 作者推荐的 VM 阅读路线（由简入深）

掌握下面四块后，`mm/` 里反复出现的模式会清晰很多：

| 顺序 | 主题 | 源码入口 | 全书章节 | 为什么先读它 |
|:----:|------|----------|----------|--------------|
| **1** | **OOM（内存耗尽）管理器** | `mm/oom_kill.c` | [Ch 13](./chapter-13-内存耗尽管理.md) | 牵涉面广但逻辑相对 **温和**，是窥见 VM 一角的好入口 |
| **2** | **非连续内存分配器** | `mm/vmalloc.c` | [Ch 7](./chapter-07-非连续内存分配.md) | 功能 **基本封装在一个文件**，较独立 |
| **3** | **物理页分配器** | `mm/page_alloc.c` | [Ch 6](./chapter-06-物理页分配.md) | 代码 **相对集中**；伙伴系统 / zone 都在这里 |
| **4** | **VMA 与进程内存区域** | `mm/mmap.c` 等 | [Ch 4](./chapter-04-进程地址空间.md) | 与用户态 `mmap`、进程地址空间直接相关 |

```
推荐阅读流（本书作者 · 非 HFT 捷径）：

  oom_kill.c  →  vmalloc.c  →  page_alloc.c  →  mmap / VMA (Ch 4)
       │              │              │                    │
     Ch 13          Ch 7           Ch 6                 Ch 4
```

### 与本仓库 HFT 精读捷径的对照

[README · HFT 精读捷径](./README.md#hft-精读捷径) 按 **性能相关** 排序：

```
Ch 2 → Ch 3 (+ THP) → Ch 8 → Ch 4 → Ch 10
```

| | **Ch 1 作者路线** | **HFT 捷径** |
|---|-------------------|--------------|
| 目标 | 第一次 **读源码** 不迷路 | **NUMA / 大页 / slab / 布局** 理论依据 |
| 起点 | OOM → vmalloc → page_alloc | 物理内存描述 → 页表 → slab |
| 关系 | 补 **「打开 `mm/` 从哪 FILE 开始」** | 补 **「为什么 HFT 要 mlock / 大页 / 绑 NUMA」** |

两条路 **不矛盾**：可按 HFT 捷径读 **章节**；读 **源码** 时按上表四个 FILE 切入。

→ 用户态 API：[08-TLPI](../08-The-Linux-Programming-Interface/) · 内核总览：[05-LKD](../05-Linux-Kernel-Development/) · [06-ULK](../06-Understanding-Linux-Kernel/) Ch 8–9

---

## 5. 提交补丁 (Submitting Patches)

把改动合入 **主线 (mainline)** 的要点：

| 必须 | 说明 |
|------|------|
| **`Documentation/CodingStyle`** | 缩进、命名、注释风格 |
| **`Documentation/SubmittingPatches`** | 补丁格式、changelog、`Signed-off-by` |
| **发对邮件列表** | **尽早、频繁** 发到 **子系统列表**（VM 相关补丁 → **linux-mm** 等），常 **Cc LKML** 引发讨论 |
| **维护者层级** | 子系统 **maintainer / lieutenant** 审查 → **推荐给 Linus** 合入 |

**文化：** 内核是 **邮件列表驱动** 的；补丁是 **对话的单位**，不是只扔一个 PR 就结束（现代部分子系统也接受 **GitLab/GitHub** 镜像，但 mm 传统仍以列表为主）。

---

## 本章带走的三句话

1. **工具：** `diff`/`patch`（或 git）+ 在线交叉引用（Elixir/LXR）+ 必要时 call graph。  
2. **读 `mm/`：** 别从 arch init 硬啃；**OOM → vmalloc → page_alloc → VMA** 由简入深。  
3. **改内核：** 先 `CodingStyle` + `SubmittingPatches`，补丁发到 **对的 mm 列表**。

---

## 相关章节

- 下一章（VM 正文起点）：[chapter-02-描述物理内存.md](./chapter-02-描述物理内存.md)
- 附录 A（Code Commentary · 简介）：[appendix-A-简介.md](./appendix-A-简介.md)

---
