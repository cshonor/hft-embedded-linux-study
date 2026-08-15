# RCU 基础 — Read-Copy-Update 机制

> **原文:** [What is RCU?](https://lwn.net/Articles/262464/) (LWN, Paul McKenney)
> **内核版本:** 2.6 (基础版) → 6.x (Tree RCU + 多种变体)
> **对标旧书:** ULK3 Ch5 (RCU 基础版) / LKD3 Ch9 (RCU 概述)

---

## 核心观点

RCU (Read-Copy-Update) 是 Linux 内核中最重要的**读多写少**同步机制。它实现了**无锁读** + **延迟回收写**，在读取路径上零开销。

### RCU 的三个基本组件

| 组件 | 说明 |
|------|------|
| **Reader (读端)** | 用 `rcu_read_lock()` / `rcu_read_unlock()` 包裹临界区，**不加锁** |
| **Writer (写端)** | 创建新副本，用 `rcu_assign_pointer()` 原子替换指针 |
| **Reclaimer (回收端)** | 等待所有现有 reader 退出后，用 `synchronize_rcu()` 回收旧数据 |

### RCU 的核心原则

1. **读取无锁**：reader 不获取任何锁，不阻塞 writer
2. **写入复制**：writer 先创建新副本，再原子替换指针
3. **延迟回收**：旧数据在所有"宽限期" (grace period) 前的 reader 都退出后才释放

### 宽限期 (Grace Period)

```
时间线:
  reader A: --rcu_read_lock()--- rcu_read_unlock()--
  reader B: ----rcu_read_lock()------ rcu_read_unlock()--
  writer:   ---------rcu_assign_pointer()---------synchronize_rc()----kfree(old)--
                                        |<--- grace period --->|
```

宽限期 = 从 writer 替换指针后，等到所有在替换前已进入 RCU 读临界区的 reader 都退出。

### 关键 API

```c
// 读端
rcu_read_lock();
p = rcu_dereference(global_ptr);  // 读端获取指针
// 使用 p...
rcu_read_unlock();

// 写端
new = kmalloc(sizeof(*new), GFP_KERNEL);
*new = *old;
new->field = new_value;
rcu_assign_pointer(global_ptr, new);  // 原子替换
synchronize_rcu();                     // 等待宽限期
kfree(old);                            // 安全释放
```

### 内核中的 RCU 应用场景

| 场景 | 用法 |
|------|------|
| **链表遍历** | `list_for_each_entry_rcu()` 无锁遍历 |
| **链表修改** | `list_add_rcu()` / `list_del_rcu()` + `synchronize_rcu()` |
| **VFS dentry cache** | 无锁查找目录项 |
| **网络路由表** | 无锁路由查找 |
| **模块卸载** | `synchronize_rcu()` 确保安全卸载 |

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 | 差异 |
|-----------|-------------|------|
| 基础 RCU (单一宽限期) | Tree RCU (层级化宽限期) | 大规模系统扩展性改进 |
| `synchronize_rcu()` 简单实现 | Tree RCU + expedited RCU | expedited 模式更快但更耗 CPU |
| 无 SRCU | Sleepable RCU (SRCU) | 允许在 RCU 读临界区睡眠 |
| 无 Tasks RCU | Tasks RCU / Tasks Trace RCU | 用于 voluntary RCU |
| `call_rcu()` 简单 | `call_rcu()` + lazy RCU | 6.x 引入延迟批量回收 |

---

## HFT 关联

| 场景 | RCU 的作用 |
|------|-----------|
| **行情数据热路径** | 无锁读取共享行情数据，writer 更新行情时用 RCU 替换 |
| **减少尾延迟** | RCU 读路径零开销，不会因锁竞争导致延迟毛刺 |
| **注意** | `synchronize_rcu()` 可能等待数十毫秒，**绝不能在交易热路径调用** |

```c
// HFT 行情数据 RCU 模式示例
struct market_data {
    struct rcu_head rcu;
    int symbol_id;
    double bid[N_LEVELS];
    double ask[N_LEVELS];
};

// 读端（交易线程，无锁）
rcu_read_lock();
md = rcu_dereference(g_market_data);
// 使用 md->bid / md->ask...
rcu_read_unlock();

// 写端（行情线程，非热路径）
new_md = kmalloc(sizeof(*new_md), GFP_KERNEL);
memcpy(new_md, old_md, sizeof(*new_md));
new_md->bid[0] = new_bid;
rcu_assign_pointer(g_market_data, new_md);
call_rcu(&old_md->rcu, market_data_free);  // 异步回收，不阻塞
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** RCU 读取路径为什么可以不加锁？writer 替换指针时 reader 正在使用旧数据怎么办？

> reader 通过 `rcu_dereference()` 获取指针，然后使用数据。writer 通过 `rcu_assign_pointer()` 原子替换指针，但**不立即释放旧数据**。旧数据在所有在替换前进入读临界区的 reader 都退出后（宽限期结束后）才被释放。因此 reader 始终操作的是有效数据，无需加锁。

**Q2:** `synchronize_rcu()` 和 `call_rcu()` 的区别？什么时候用哪个？

> `synchronize_rcu()` 是同步等待宽限期结束，调用者会阻塞。`call_rcu()` 是注册回调，宽限期结束后异步调用回调释放数据。热路径或不能阻塞的场景用 `call_rcu()`；初始化/清理路径可用 `synchronize_rcu()`。

**Q3:** 在 RCU 读临界区内能睡眠吗？能调用 `kmalloc(GFP_KERNEL)` 吗？

> 经典 RCU (`rcu_read_lock()`) **不能睡眠**——它依赖禁用抢占来保证读临界区不会跨宽限期。在 RCU 读临界区内调用 `kmalloc(GFP_KERNEL)` 可能睡眠，是 BUG。如果需要睡眠，用 SRCU (`srcu_read_lock()`)。

</details>
