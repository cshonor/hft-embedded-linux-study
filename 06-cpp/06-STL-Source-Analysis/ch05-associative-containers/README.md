# 第 5 章 关联容器

**Associative Containers**

## 本章讲什么

STL 关联容器的底层只有两棵树（红黑树、哈希表）。`set`/`map` 是红黑树的封装，`unordered_*` 是哈希表的封装。本章剖析红黑树的平衡性质与哈希表的开链结构，解释它们的查找复杂度根源。

## 要点

### 红黑树（RB-tree）：平衡二叉搜索树

红黑树 5 性质保证高度 O(log n)：
1. 节点红或黑
2. 根黑
3. 叶（NIL）黑
4. 红节点子必黑（无连续红）
5. 任一节点到叶的各路径黑节点数相同（黑高）

**插入/删除**可能破坏平衡，通过**旋转 + 重新着色**恢复，最多 2/3 次旋转。查找 O(log n)。

### `set`/`map` 是 RB-tree 的封装

`set<T>` = `rb_tree<T, T, identity<T>, less<T>>`（键值合一）；`map<K,V>` = `rb_tree<pair<const K,V>, ...>`。`map` 的元素是 `pair<const K, V>`——键 const 保证不可改。

### 哈希表（hashtable）：开链法

```cpp
vector<list<node>> buckets;   // 每个桶是一条链表
size_t bucket = hash(key) % buckets.size();
```
- 查找：哈希定位桶 O(1) + 链表遍历 O(桶长)。
- 负载因子 = 元素数 / 桶数；超阈值 rehash（桶数翻倍 + 重哈希）。
- SGI 用素数桶数（减少聚集），`unordered_*`（C++11）标准化。

### `hash_set`/`hash_map` → `unordered_*`

SGI 的 `hash_set`/`hash_map` 是非标准扩展，C++11 标准化为 `unordered_set`/`unordered_map`。

## HFT 关联

- **红黑树 vs 跳表 vs 哈希**：订单簿价格档位需要有序范围查询（最优买/卖、档位聚合），用红黑树（`map`）或跳表；纯键查找用哈希（`unordered_map`）。HFT 订单簿常用自定义红黑树/跳表而非 `std::map`（控制分配 + cache）。
- **`unordered_map` rehash 尖峰**：启动预 `reserve` 桶数，避免热路径 rehash。
- **开链 vs 开放寻址**：STL 用开链（链表），HFT 高负载因子下链表变长，有些 HFT 哈希表用开放寻址 + 线性探测换 cache 友好（见 `rte_hash`）。

## 自测题

1. 红黑树的 5 条性质是什么？它们如何保证高度 O(log n)？
2. `set` 和 `map` 分别如何封装 RB-tree？`map` 的键为什么是 `const`？
3. 哈希表开链法的负载因子是什么？超阈值发生什么？
4. SGI 的 `hash_map` 在 C++11 标准化为什么？
5. 订单簿价格档位为什么用红黑树/跳表而非哈希？纯键查找用什么？
