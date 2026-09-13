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
3. **所有叶子（NIL）都是黑色**（“叶子” = NIL 哨兵，**不是**存数据的真实节点）；  
4. 红节点的两个子节点必须黑（**不能连续红**）；  
5. 任意节点到其所有后代叶子，路径上 **黑色节点数相等**（黑高一致）。

作用：最长路径 ≤ 最短路径 × 2 → 树不退化成链表 → 维持 **O(log n)**。

| 操作 | 复杂度 |
|------|--------|
| 插入 / 删除 / 搜索 | **O(log n)** |

插入/删除破坏规则时：靠 **变色 + 左旋/右旋** 修复。内核 `rbtree.c` 核心就是这套逻辑。

---

### NIL 哨兵 · 黑高 · 插入修复（教科书版 vs 内核版）

上面五条是**定义**；这一段是**实现**。最容易被一句话带过、也最该讲清的是三件事：空孩子怎么表示、黑高怎么数、“不能连续红”到底在防什么。

同一件事，两种相反写法：

```c
/* ① 教科书（CLRS / 多数算法书）:一个哨兵对象 */
typedef struct Node {
    int           key;
    int           color;            /* RED / BLACK */
    struct Node  *left, *right, *parent;
} Node;

static Node NIL_NODE = { .color = BLACK };   /* 唯一的哨兵，全树共享 */
#define NIL (&NIL_NODE)

/* 叶子写 NIL 而不是 NULL —— 于是这类函数可以直接吃 NIL，
   不用到处 if (p == NULL) */
static int is_red(Node *n) { return n->color == RED; }

/* ② Linux 内核 v6.6:根本没有哨兵 */
struct rb_node {
    unsigned long  __rb_parent_color;        /* 父指针 + 颜色 → 一个字段 */
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
```

（内核结构见 `include/linux/rbtree_types.h:5-8`。）

#### 教学版 vs 内核版：逐条对上

| 问题 | 教科书（CLRS）写法 | Linux 内核 v6.6 |
|------|-------------------|-----------------|
| 空孩子怎么表示 | 全部指向唯一的哨兵 `T.nil` | 就是 **`NULL`** |
| 哨兵是真实对象吗 | **是**——一个真实分配的节点，但不存业务数据 | **不存在**。`RB_EMPTY_ROOT(root)` 就是 `root->rb_node == NULL`（`rbtree.h:30`） |
| 颜色存哪 | 独立字段 `color` | **没有独立字段**——塞进 `__rb_parent_color` 低 2 位（`rbtree_augmented.h:171-172`） |
| 红/黑怎么编码 | 各家不一（`RED=1`、`RED=0` 都有） | **`RB_RED 0` / `RB_BLACK 1`** —— 红 = 0 |
| 新节点初始色 | 显式 `color = RED` | `rb_link_node()` 只写 `__rb_parent_color = (unsigned long)parent`，**低 2 位天然为 0 = 红**（`rbtree.h:59-66`） |
| “未插入”怎么标 | 哨兵兼任 | `RB_EMPTY_NODE(node)`：`__rb_parent_color == (unsigned long)node`（**自指**，`rbtree.h:33-34`） |
| 官方自述的理由 | 统一边界、少写判空 | *"one less layer of indirection (and better cache locality) than more traditional tree implementations"*（`Documentation/core-api/rbtree.rst:50-53`） |

> ⚠️ 所以“红黑树都用 NIL 哨兵、不用裸 NULL”是**教学版的约定，不是红黑树的性质**。内核和 libstdc++ 的**空孩子都是裸 `NULL`**。
> 另一个高频记混点：**“红 = 0”才是主流**——libstdc++ 也是 `enum _Rb_tree_color { _S_red = false, _S_black = true };`（`bits/stl_tree.h:95`）。“0 = 黑、1 = 红”是反的。
>
> **补充：libstdc++ 是第三种写法**——不用叶子哨兵，但另有**一个 `_M_header` 结点**：`end()` 指向它，它的 `_M_parent` 是根、`_M_left` 是最左、`_M_right` 是最右（`stl_tree.h:709/994`，重置在 `_M_reset` :201-207），而且它的颜色被刻意设成**红**（:171）。这和内核把最左结点缓存在 `rb_root_cached.rb_leftmost`（`rbtree.h:105-106`，`rb_first_cached` 因此 O(1)）是**同一个优化动机，只是载体不同**。

#### “红 = 0”不是随手选的

```c
/* 红节点的 __rb_parent_color 低 2 位本来就是 0 —— 直接当裸指针用 */
static inline struct rb_node *rb_red_parent(struct rb_node *red)
{
	return (struct rb_node *)red->__rb_parent_color;
}                                          /* lib/rbtree.c:64-67 */

/* 黑节点低位是 1，才需要把低 2 位抹掉 */
#define rb_parent(r)  ((struct rb_node *)((r)->__rb_parent_color & ~3))   /* rbtree.h:26 */
```

`rb_node` 至少按 `sizeof(long)` 对齐 → 父指针低 3 位恒为 0。于是**取红节点的父指针一次运算都不用**，取黑节点的才要 `& ~3`。这是“把颜色塞进指针低位”的副作用，也是它值得塞的原因。

#### 黑高到底怎么数

CLRS 原文：

> We call the number of black nodes on any path from, but **not including**, a node x to a leaf the **black-height** of the node, denoted bh(x).
> ⋯ All NILs have black-height 0, and the black-height of the tree in Figure 13.1 is 3.

三条：

1. **起点（x 自己）不算** —— 原文 “but not including”；
2. 路径终点是**叶子**，而 CLRS 的“叶子”就是 NIL、NIL 是黑的 → 走到底那个 NIL **也算一个黑**；
3. “NILs have black-height 0” 说的是 **NIL 自己当起点时为 0**，不能反推“父节点的路径上不含 NIL”。

⚠️ 也有教材把 NIL 排除在计数之外。**口径只让数值差 1，不影响“各路径相等”这个不变量**——同一棵树、同一道题里保持一致即可。

按上面的口径，一棵 7 节点树：

```
        B   ← 根
      /   \
     R     R
    / \   / \
   B   B B   B
  / \ / \ / \ / \
 N   N N N N N  N
```

路径 `B(根) → R → B → NIL` 的黑节点 = 中间那个 **B** + **NIL** = **2**（不是 3——3 是“把起点根也算上”的口径）。两种口径都对，**但别在同一个例子里混用**。

#### “不能连续两个红”到底在防什么

常见说法是“红红相连会破坏黑高”。**这句不准确**——存在黑高完全相等、却连着两个红的树。最小反例（下面枚举器的第一个命中）：

```
B          ← 根黑，② ✅
 \
  R        ← 红
   \
    R      ← 红，④ ❌
```

四条路径 `B→NIL` / `B→R→NIL` / `B→R→R→NIL`（×2）的黑节点数（不含根、含 NIL）**全是 1** → ⑤ 满足，④ 被违反。

那 ④ 的真正作用是什么：**把高度压住**。⑤ 只管“各路径相等”，而一条纯链的每一层子树都只有一条路径 → ⑤ 恒真、完全没有约束力。枚举全部 n 节点二叉树 × 全部 2ⁿ 种着色（高度按“根到 NIL 的层数”计，即 `height(NIL) = 0` 的递归口径）：

| n | 满足 ②③⑤ 的树 | 其中违反 ④ | ②③⑤ 允许的最大高度 | ②③④⑤ 允许的最大高度 |
|---|--------------|-----------|-------------------|---------------------|
| 3 | 6 | **4** | 3 | 2 |
| 4 | 18 | 14 | 4 | 3 |
| 5 | 58 | 50 | 5 | 3 |
| 6 | 192 | 176 | 6 | 4 |
| 7 | 654 | 621 | 7 | 4 |
| 8 | 2270 | 2214 | **8** | **4** |

读法：**去掉 ④，最大高度随 n 线性增长（n = 8 时就是一条 8 节点链）；加上 ④ 才压到 ~2·log₂(n+1)。**

> 正确表述：⑤ 保证“各路径黑节点数相同”，④ 保证“红不连续” → **两件事合起来**才推出
> 「最长路径 ≤ 2 × 最短路径」→ 树高 ≤ 2·log₂(n+1)。
> **④ 不是 ⑤ 的推论**，它俩是并列的两条。

#### 插入修复：内核源码里的 Case 1 / 2 / 3

新节点一定是红的——内核里连“涂红”这行都不用写（`rb_link_node()` 那个赋值的低 2 位天然是 0）：

```c
static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
				struct rb_node **rb_link)
{
	node->__rb_parent_color = (unsigned long)parent;   /* 低 2 位 = 0 → 红 */
	node->rb_left = node->rb_right = NULL;
	*rb_link = node;
}                                    /* include/linux/rbtree.h:59-66 */
```

`lib/rbtree.c` 的 `__rb_insert()` 就是教科书那三情形，注释里 ASCII 图都画好了：

| 情形 | 条件 | 动作 | 源码 |
|------|------|------|------|
| 父黑 | `rb_is_black(parent)` | **直接结束**（④ 没破） | `rbtree.c:110-111` |
| **Case 1** | 叔叔是**红** | 父、叔涂黑、祖父涂红；**把祖父当新起点继续上溯** | `rbtree.c:117-137` / `185-193` |
| **Case 2** | 叔黑，且 node 是父的**内侧**孩子（之字形） | 在**父**处旋转一次 → **转成 Case 3**（本身不解决问题） | `rbtree.c:140-164` / `196-208` |
| **Case 3** | 叔黑，且 node 是父的**外侧**孩子（一字形） | 在**祖父**处旋转；父变黑、祖父变红 → **结束** | `rbtree.c:166-182` / `210-217` |

- **Case 1 是唯一会循环的一支**：变色后 `node = gparent; continue;`——祖父的新父亲可能又是红的，上溢继续传。
- **Case 2 → Case 3 走完就 `break`**：所以**插入修复最多 2 次旋转**（“之”字形一次 + 祖父处一次）。
- 旋转用 `__rb_rotate_set_parents(gparent, parent, root, RB_RED)`（`rbtree.c:180` / `215`）——把祖父的身份交给父，同时把祖父涂红。

| 名词 | 一句话 |
|------|--------|
| NIL 哨兵 | 教科书用**真实对象**代表空孩子；内核**不用**，`NULL` 就够 |
| 黑高 bh | 某节点（不含自己）到叶子路径上的黑节点数；每条路径相等 |
| ④ 不能连续红 | **不被 ⑤ 蕴含**；把高度从 O(n) 压到 O(log n) 的是它 |
| 新节点颜色 | 红；内核里“红 = 0”，`rb_link_node()` 一次赋值同时写好父指针和颜色 |
| 插入修复 | 叔红 → 变色上溯（可循环）；叔黑 → 先转（Case 2）再旋（Case 3），最多 2 次旋转 |

---

### 内核 `rbtree` 特点（源码阅读重点）

路径：`include/linux/rbtree.h`（及实现 `lib/rbtree.c` 等）。

和教科书常见实现的细微差别：

#### 1. 节点里不存业务数据 — 内嵌 + `container_of`

```c
/* include/linux/rbtree_types.h:5-8 (v6.6) */
struct rb_node {
    unsigned long  __rb_parent_color;   /* 父指针 + 颜色(低 2 位), 一个字段 */
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

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

统一用 **NULL** 代表叶子——**没有**教科书那种 `T.nil` 哨兵对象。
完整对照（含“颜色为什么塞进指针低位”）见上文《NIL 哨兵 · 黑高 · 插入修复（教科书版 vs 内核版）》。
一句话：内核宁可多写几处判空，也不愿为哨兵多付一层间接——官方理由见 `Documentation/core-api/rbtree.rst:50-53`。

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
5. 约束核心：禁连续红、黑高均衡；
6. **NIL 哨兵只是教学约定**——内核与 libstdc++ 的空孩子都是裸 `NULL`，颜色塞进指针低位的写法比教科书少见。

---

### 自检

**`rb_first(cfs_rq->rb_root)` 拿到的是什么？**

→ 该 CPU 上 CFS 就绪树里 **`vruntime` 最小** 的那个 `rb_node`（再 `container_of` 成 `sched_entity` / 任务）— 即 **下一个（或当前应优先）被 CFS 选中运行的调度实体**。

**黑高的“起点”算不算？** 按 CLRS：不算（"but not including"），但走到底的 NIL 要算。口径差 1，不影响“各路径相等”。

**为什么“不能连续红”不是“黑高相等”推出来的？** 因为纯链每层子树只有一条路径，黑高恒相等——④ 必须单独规定，它才是把高度压到 O(log n) 的那条。

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

**Q3.** NIL 哨兵是真实存在的“数据节点”吗？Linux 内核里有这个哨兵吗？

<details><summary>答案</summary>

分两层答：

- **教学版（CLRS）**：`T.nil` **是一个真实存在的对象**（真实分配的一个节点），但它**不是数据节点**——不存 key/value，只当边界标记；全树共享同一个。
- **Linux 内核 v6.6**：**根本没有哨兵**。空孩子就是 `NULL`（`rb_link_node()` 里 `rb_left = rb_right = NULL`），`RB_EMPTY_ROOT()` 就是判 `rb_node == NULL`。

所以“红黑树用 NIL 哨兵”是**教学实现的选择**，不是红黑树的性质。

</details>

**Q4.** 只要求“根黑 + 叶子黑 + 黑高相等”（去掉“不能连续红”），树高还有 O(log n) 上界吗？

<details><summary>答案</summary>

**没有了。** 纯链（每个节点只有一个孩子）的每层子树都只有一条路径，黑高天然相等，⑤ 完全不起约束作用 → 高度可到 O(n)。
实测枚举：n = 8 时满足 ②③⑤ 的树最大高度是 **8**（一条 8 节点链），加上 ④ 才降到 **4**。

</details>

**Q5.** 内核里新插入的节点，是在哪一行被“涂成红色”的？

<details><summary>答案</summary>

**没有单独的一行。** `rb_link_node()` 里 `node->__rb_parent_color = (unsigned long)parent;` —— 因为 `RB_RED = 0` 且父指针低 2 位为 0，这一次赋值就同时写好了父指针和“红”。
（若新节点成为根，`__rb_insert()` 第一个分支 `rb_set_parent_color(node, NULL, RB_BLACK)` 再给它涂黑。）

</details>

</details>
---
