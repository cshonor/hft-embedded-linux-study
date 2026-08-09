## ④ 二叉树 · 红黑树 · `rbtree`

结合 **Linux CFS** 来讲，不单纯背算法题。

---

### CFS 为什么用红黑树？

CFS 反复要做：

**快速找到 `vruntime` 最小的调度实体**；任务加入 / 移出；`vruntime` 更新后重新排序。

| 结构 | 问题 |
|------|------|
| 普通链表 | 找最小值 **O(n)**，太慢 |
| AVL | 平衡严、旋转频繁 |
| **红黑树** | 查/插/删 **O(log n)**；平衡规则相对松，旋转更少 |

内核通用实现：`include/linux/rbtree.h`。  
CFS 的 **`cfs_rq` 就绪队列底层就是它**。

> **二叉搜索树性质永远优先：**  
> 左子树 key < 当前 < 右子树。  
> CFS 里 **key = `vruntime`** → **最左节点 = `vruntime` 最小** → 下一个该跑的任务。

→ 调度语义：[Ch4 §4.3 CFS](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md)

---

### 五大硬性约束（标准定义）

1. 每个节点非黑即红；  
2. **根一定是黑色**；  
3. **所有叶子（NIL）都是黑色**；  
4. 红节点的两个子节点必须黑（**不能连续红**）；  
5. 任意节点到其所有后代叶子，路径上 **黑色节点数相等**（黑高一致）。

作用：最长路径 ≤ 最短路径 × 2 → 树不退化成链表 → 维持 **O(log n)**。

| 操作 | 复杂度 |
|------|--------|
| 插入 / 删除 / 搜索 | **O(log n)** |

插入/删除破坏规则时：靠 **变色 + 左旋/右旋** 修复。内核 `rbtree.c` 核心就是这套逻辑。

---

### 内核 `rbtree` 特点（源码阅读重点）

路径：`include/linux/rbtree.h`（及实现 `lib/rbtree.c` 等）。

和教科书常见实现的细微差别：

#### 1. 节点里不存业务数据 — 内嵌 + `container_of`

```c
struct rb_node {
    unsigned long  rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

/* 概念：rb_node 嵌进 sched_entity */
struct sched_entity {
    struct rb_node run_node;
    u64 vruntime;
    /* ... */
};
```

由 `rb_node *` 反推外层 `sched_entity *`，靠 **`container_of`**（依赖 GCC `typeof` 等扩展）。

→ [§6.2 `container_of`](./section-6.2-链表.md) · [Ch2 §2.4 GNU C](../../chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)

#### 2. 无专门 NIL 哨兵对象

统一用 **NULL** 代表叶子。

#### 3. 标准接口（手写比较）

C **无泛型** → **没有**「插入任意类型」的万能函数；开发者用 `rb_*` 辅助 **自己写** 比较/插入/删除。

| 接口（概念） | 用途 |
|--------------|------|
| 插入 / 删除 / 查找 | 按自定 key 比较 |
| **`rb_first()`** | 拿最左节点（CFS：最小 `vruntime`） |

收益：比较可内联、少一层间接 — 热路径友好。

---

### 套回 CFS 完整流程

1. **新任务就绪**：以 `vruntime` 为 key，插入 `cfs_rq` 的红黑树；  
2. **选任务**：`rb_first(cfs_rq 的树根)` → 最左 = 最小 `vruntime`；  
3. **跑一段时间后被切走**：从树 **删除** → 更新 `vruntime` → **再插入**（key 变了要重排）；  
4. **休眠 / 退出就绪队列**：从树 **erase**。

```
就绪入树 ──► rb_first 选跑 ──► 记账涨 vruntime ──► 删+改+插回树
                │
                └─ 休眠：直接 erase
```

---

### 极易混淆

#### 红黑树 ≠ 堆

| | 最小堆 | 红黑树（CFS 选型） |
|--|--------|-------------------|
| 取最小值 | 很快 | 最左 **O(log n)** 也可接受 |
| **key 频繁变化后重排** | 难高效调整位置 | **适合动态改 key** 再插回 |

CFS 任务运行中 `vruntime` **持续增长**，需要频繁改 key 重排 → 二叉搜索树更合适。

#### 旋转

只有两种基础：左旋、右旋。破坏五条规则 → 变色 + 旋转修复。

---

### 极简记忆（内核向）

1. 本质：自平衡 BST，**O(log n)**；  
2. CFS：key = `vruntime`，管就绪 `sched_entity`；  
3. **最左 = 下一个要跑的进程**；  
4. 内核：内嵌 **`rb_node` + `container_of`**；  
5. 约束核心：禁连续红、黑高均衡。

---

### 自检

**`rb_first(cfs_rq->rb_root)` 拿到的是什么？**

→ 该 CPU 上 CFS 就绪树里 **`vruntime` 最小** 的那个 `rb_node`（再 `container_of` 成 `sched_entity` / 任务）— 即 **下一个（或当前应优先）被 CFS 选中运行的调度实体**。

---

### 选型对照

→ [§6.6](./section-6.6-选择合适的数据结构.md) · [§6.7 复杂度](./section-6.7-算法复杂度.md) · [Ch4 CFS](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 红黑树为什么被 CFS 调度器选用？

<details><summary>答案</summary>

CFS 需要快速找到 vruntime 最小的进程（左下角节点）+ 快速插入/删除。红黑树：查找 O(log n)、插入 O(log n)、删除 O(log n)、找最小值 O(log n)。AVL 树更平衡但插入/删除旋转更多；B 树适合磁盘但内存中红黑树更简单。CFS 的 rbtree 缓存了最左节点，找最小值 O(1)。

</details>

**Q2.** rbtree 和 B+ 树在什么场景下各自更优？

<details><summary>答案</summary>

rbtree：内存中、少量数据（万级）、频繁插入/删除。B+ 树：磁盘上、大量数据（百万级）、顺序扫描多。内核 VFS 的目录项用 rbtree（内存）；数据库索引用 B+ 树（磁盘，减少 IO 次数）。HFT 的限价单簿在内存中，通常用 rbtree 或哈希表。

</details>

</details>
---
