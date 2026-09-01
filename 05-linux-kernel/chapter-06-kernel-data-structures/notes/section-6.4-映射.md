## ③ 映射 · Maps · `idr`

**关联数组** — 将 **唯一键** 映射到 **值**。

Linux **`idr`** 针对的用例：

| 特点 | 说明 |
|------|------|
| 键 | **唯一整数 UID**（常自动分配） |
| 值 | **`void *` 指针**（指向内核对象） |
| 操作 | 添加、删除、查找；添加时可 **自动分配新 UID** |

#### 两步分配（3rd 版书中 API）

| 步骤 | 函数 |
|------|------|
| 1 | **`idr_pre_get()`** — 预分配内存 |
| 2 | **`idr_get_new()`** — 分配 UID 并插入 |

**为什么要两步——"不能失败的分配"设计**：

```
传统一步式:  idr_get_new() 内部需要扩树 → 分配节点 → 分配可能失败
             → 调用者拿到"分配到一半"的中间状态，回滚麻烦

两步式:      idr_pre_get()   ← 在锁外、允许失败（返回 0 = 没分到，可重试）
             idr_get_new()   ← 在锁内、绝不分配（用预分配的存货）→ 不可能失败
```

| 设计原则 | 体现 |
|----------|------|
| **锁内不做可能失败/睡眠的操作** | 与 maple tree 的 `vma_iter_prealloc`（[15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)）、GFP_ATOMIC 预留水位（[12.1](../../chapter-12-memory-management/notes/section-12.1-为何内核内存更复杂.md)）**同一个思想在三处出现** |
| 循环重试模板 | `while (!idr_pre_get(...)) ; /* 或 -ENOMEM 退出 */ idr_get_new(...)` |

> 现代内核已把这套仪式简化：`idr_alloc()`（一步、内部 GFP 管理失败）直接返回 ID 或负错误码——**"两步"是历史 API**，但其"预分配去失败化"的思想仍是内核并发设计的通用武器。

#### 基数树为什么适合 ID → 指针

| 性质 | 说明 |
|------|------|
| 键是**稀疏整数** | ID 可能跨 0..INT_MAX，但实际只用一小撮——按位每 6~7 bit 一层，**只为存在的分支分配节点** |
| 查找 = 位移 + 数组下标 | `key >> shift & mask` 每层一次——**没有哈希函数、没有冲突链**，最坏 O(层数) |
| 键序即字典序 | 可做 `idr_for_each` 范围遍历（哈希表做不到） |

> **注：** 新内核中 `idr` 接口有演进，并广泛使用 **`xarray`（xa）** 等结构 — 读书时抓 **「整数 ID → 指针」** 语义即可。

#### xarray：idr 的现代统一者（4.20+）

| 维度 | idr（旧） | xarray（新） |
|------|-----------|--------------|
| API 风格 | 两步分配仪式 | `xa_alloc` / `xa_store` / `xa_load` 一步式，返回错误码 |
| 并发 | 调用方全权加锁 | **内置 `xa_lock`**（初始化时可选关掉走自定义 RCU） |
| 复用 | 与 radix tree 各一套代码 | 统一底层数据结构——**page cache 已从 radix tree 迁到 xarray**（著名案例，Matthew Wilcox 主导） |
| 标记 | 无 | **每条目 3 个 tag bit**（如 page cache 的 dirty/writeback 标记直接挂在树上）——遍历时按 tag 反查极快 |

| 典型用途 | 示例 |
|----------|------|
| 小整数 fd 式句柄 | 驱动 minor、内部 handle 表 |
| **page cache** | 页索引 → page 指针（tag 标脏页，回写引擎按 tag 批量扫）——与 [Ch 16 页缓存](../../chapter-16-page-cache/notes/section-16.3-address_space-与基数树.md)衔接 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** idr 的典型用途是什么？为什么不用哈希表？

<details><summary>答案</summary>

idr 用于分配唯一整数 ID 并快速查找。典型用途：分配 inode 号、设备号、POSIX 定时器 ID。不用哈希表是因为：1) 需要紧凑分配（从 0 开始不跳跃）；2) 需要 O(1) 查找 + O(1) 分配/释放；3) 基数树（radix tree）实现，比哈希表更 cache 友好。

</details>

**Q2.** idr 和现代内核的 xarray 有什么关系？

<details><summary>答案</summary>

xarray 是 idr 的现代替代（Linux 4.20+），API 更简洁、性能更好。`xa_alloc()` 替代 `idr_alloc()`，`xa_load()` 替代 `idr_find()`。idr 仍保留兼容但新代码推荐 xarray。page cache 已从 radix tree 迁移到 xarray。

</details>

**Q3.** idr 的"两步分配"解决什么问题？这个思想还在哪些地方出现？

<details><summary>答案</summary>

解决"**持锁路径中分配可能失败**"：老 API `idr_pre_get()` 在锁外预分配树节点（允许失败可重试），`idr_get_new()` 在锁内只消费存货、不可能失败——避免锁内回滚。同一思想的三处现代现身：① maple tree 的 `vma_iter_prealloc()`（锁外预分配节点再 `vma_iter_store`）；② GFP_ATOMIC 紧急预留水位（中断路径不睡眠）；③ 用户态临界区不做 malloc 的纪律。**把可能失败的操作挪到锁外，锁内只剩确定成功的提交**。

</details>

</details>
---
