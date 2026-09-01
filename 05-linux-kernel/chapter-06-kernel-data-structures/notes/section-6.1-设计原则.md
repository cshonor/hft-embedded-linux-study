## 设计原则

内核提供通用数据结构原语 → **鼓励重用**。

| 建议 | 原因 |
|------|------|
| **用内核现成结构** | 经审计、一致、少 bug |
| **勿 roll your own** | 链表/树写错一处 → 难查的内存破坏 |

#### 泛型怎么来的：嵌入 + 宏，不是模板

C 没有 C++ 模板，内核的"泛型数据结构"靠**两条腿**：

```
① 结构内嵌：把 list_head 塞进宿主对象
   struct task_struct {
       ...
       struct list_head tasks;    ← 节点即成员，不是"节点指向数据"
   };

② container_of 反推：从成员地址算回宿主地址
   list_entry(ptr, struct task_struct, tasks)
       = (char*)ptr - offsetof(struct task_struct, tasks)
```

由此带来三个教科书级优点（LKD 逐一点名）：

| 优点 | 机理 |
|------|------|
| **零额外分配** | 节点不是独立 malloc 的对象——挂链表不产生新内存 |
| **一物多挂** | 一个 task_struct 同时挂 tasks 链表和 CFS 红黑树——**多个 `list_head`/`rb_node` 成员**即可 |
| **类型安全（编译期）** | `container_of` 里的 `typeof` 编译期检查宿主类型，传错类型编不过 |

> 对比：glib 的 `GList` 是"节点持有 `void*` 数据指针"模型——每挂一个元素多一次节点分配、多一层间接寻址（cache 不友好）。内核模型把链接开销**摊进宿主对象的缓存行里**。

#### 内核数据结构全家福（含现代成员）

| 结构 | 章节 | 一句话 | 现代补充 |
|------|------|--------|----------|
| **链表 `list_head`** | 6.2 | 双向循环，遍历 O(n) | `hlist`（哈希桶单指针头，省内存）、`llist`（无锁单生产者栈） |
| **队列 `kfifo`** | 6.3 | 定长环形缓冲，SPSC 可无锁 | —— |
| **映射 `idr`** | 6.4 | 整数 ID → 指针 | **`xarray`**（4.20+）接班：page cache 已迁移 |
| **红黑树 `rbtree`** | 6.5 | 有序键，O(log n) | CFS、epoll、VMA（→ VMA 已再改 **maple tree**，v6.1+，见 [15.4](../../chapter-15-process-address-space/notes/section-15.4-内存区域的链表与树.md)） |
| **哈希表 `hlist`+`hash_head`** | — | 桶式 O(1) | PID 查找、dentry 缓存 |
| **maple tree** | — | — | v6.1+ VMA 专用：B-tree 特化、范围查询、缓存友好 |

> 趋势读法：**"通用结构 + 调用方自带锁"**（LKD3rd 时代的 rbtree/list）正在向**自带并发协议**的结构演化——xarray 内置 `xa_lock`（可关）、maple tree 内置 RCU 读侧。理由：锁的粒度与数据结构的形状耦合，分开写必然写错。

#### 与用户态的镜像

| 内核 | 用户态对应 |
|------|-----------|
| `list_head` + `container_of` | `intrusive::list`（Boost）/ 手写侵入式链表 |
| `kfifo` | DPDK `rte_ring`、SPSC ring（liburcu、folly） |
| `idr`/`xarray` | 句柄表（fd 的思路）、slab 分配器的 free 管理 |
| per-CPU 数据 | thread_local / shard 化计数器 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核为什么要提供自己的链表/队列/映射，而不直接用 C 标准库？

<details><summary>答案</summary>

1) 内核没有 libc，不能用标准库；2) 内核数据结构需要考虑 SMP 安全（自旋锁/RCU）、内存效率（嵌入式设计无数据载荷指针）、实时性（O(1) 操作）。标准库的数据结构不考虑这些内核特有约束。

</details>

**Q2.** 内核数据结构的「嵌入式设计」是什么意思？

<details><summary>答案</summary>

标准链表节点包含数据指针；内核 list_head 嵌入在数据结构内部（如 struct task_struct { struct list_head tasks; }）。通过 container_of 宏从 list_head 反推宿主结构地址。优点：零额外内存分配、一个对象可挂多个链表、类型安全。

</details>

**Q3.** 为什么说"通用结构与锁分离"正在被"结构自带并发协议"取代？举两个例子。

<details><summary>答案</summary>

锁粒度与数据结构形状天然耦合，分开写容易错（该锁树的时候只锁了节点、该 RCU 的时候用了 spinlock）。例子：① **xarray** 内置 `xa_lock`（可传 NULL 关闭），插入/删除自动处理并发，调用方不再手搓 radix tree + 自旋锁两套代码；② **maple tree**（v6.1+ VMA）读侧直接走 RCU、写侧 `vma_iter_prealloc` 锁外预分配——并发协议封装进迭代器 API。对比 LKD3rd 时代：rbtree 是纯裸结构，锁完全靠调用方自觉。

</details>

</details>
---
