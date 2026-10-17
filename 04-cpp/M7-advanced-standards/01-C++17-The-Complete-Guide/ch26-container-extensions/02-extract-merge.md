# extract / merge：节点转移

## extract：零拷贝取出节点

```cpp
std::map<int, std::string> a, b;
a[1] = "one";
a[2] = "two";

// extract：取出节点，不拷贝、不分配
auto node = a.extract(1);   // node 持有 {1, "one"} 的所有权
// a 中不再有 key=1

// node 是 node_type，独占所有权
if (!node.empty()) {
    std::cout << node.key() << ": " << node.mapped();  // 1: one
}
```

## 改 key 后重新插入

```cpp
// map 的 node 可以改 key（set 不行）
auto node = a.extract(2);
node.key() = 99;               // 改 key
a.insert(std::move(node));     // 重新插入，key 变成 99
// 零拷贝、零分配——只改了 key，value 不动

// 对比 C++14 做法：
// a.erase(2); a.emplace(99, std::move(old_value));
// 有一次析构 + 一次构造 + 可能的内存分配
```

## merge：批量转移

```cpp
std::map<int, std::string> a = {{1, "one"}, {2, "two"}};
std::map<int, std::string> b = {{2, "TWO"}, {3, "three"}};

// merge：把 a 的所有节点转移到 b
b.merge(a);
// b = {{1, "one"}, {2, "TWO"}, {3, "three"}}
// a = {{2, "two"}}  ← key=2 冲突，留在 a 中
// 零拷贝转移！
```

**注意**：`merge` 只转移不冲突的节点。如果 key 在目标中已存在，该节点留在源容器中。

## node_type 特性

```cpp
auto node = a.extract(1);

// node_type 特性：
// - 可移动不可拷贝
// - empty() 检查是否为空
// - 析构时如果非空，销毁节点（不会泄漏）
// - map 的 node 可改 key()，set 的不可改

if (node.empty()) {
    // extract 失败（key 不存在）
}

// 移动赋值
auto node2 = std::move(node);  // node 变空
```

## 性能优势

```cpp
// 传统方式：erase + insert
auto it = a.find(old_key);
if (it != a.end()) {
    auto val = std::move(it->second);
    a.erase(it);
    a.emplace(new_key, std::move(val));
}
// 1 次析构 + 1 次构造 + 可能的重新平衡

// C++17 extract + 改 key + insert
auto node = a.extract(old_key);
node.key() = new_key;
a.insert(std::move(node));
// 0 次析构 + 0 次构造 + 可能的重新平衡
// 节点内存复用
```

## 自测题

1. `extract` 相比 `erase` + `insert` 有什么优势？
2. map 的 `node_type` 能改 key 吗？set 呢？为什么？
3. `merge` 在 key 冲突时怎么处理？
4. `node_type` 的所有权语义是什么？（可拷贝？可移动？）
5. 用 `extract` + 改 key + `insert` 重命名 map 中的 key，比传统方式省了什么？

<details>
<summary>参考答案</summary>

1. `extract` 把节点**整体摘下来**交给你（node handle），元素本身既不析构也不拷贝/移动，节点内存可以复用；`erase` + `insert` 则要析构旧元素、再分配新节点、再构造新元素。
所以 `extract` 的优势是：零次元素构造/析构、零次节点内存分配，还能把同一个节点搬到另一个（同类型的）容器里，甚至改完 key 再插回去。
2. **map 可以**：`node.key()` 返回 `key_type` 的非 const 引用，可以直接改。
**set 不可以**：`set` 的 `node_type` 只提供 `value()` 且是 const 的，不给 `key()`。
原因很直接：set 的元素就是 key，key 决定节点在树/哈希表中的位置，放开了改就破坏容器的序与不变式；map 的 key 与 value 分离，`extract` 后节点已脱离容器，改 key 再 insert 回去是安全的（key 是否冲突会在 insert 时重新判定）。
3. 冲突的节点**留在源容器里不动**，不会被转移到目标容器，也不会覆盖目标容器中已有的元素。
`merge` 的语义是「把源容器中那些 key 不与目标容器冲突的节点逐个转移过去」，冲突的保留在源容器，并且仍然指向原来的元素（引用/迭代器不失效）。
4. `node_type` 是**只可移动、不可拷贝**的资源句柄（move-only），符合「独占一个节点」的所有权语义。
   - 可移动：移动后源句柄变空（`empty()` 为 `true`），所有权转移。
   - 不可拷贝：拷贝构造/拷贝赋值被删除，避免两个句柄持有同一节点。
   - 若句柄在析构时仍非空，会销毁其中的元素并释放节点——不会泄漏。
   - `empty()` 可判断句柄是否为空（例如 `extract` 的 key 不存在时就拿到空句柄）。
5. 省掉了**一次元素析构 + 一次元素构造 + 一次节点内存分配**（以及对应的移动构造）：
```cpp
// 传统：erase + insert
auto it = a.find(old_key);
if (it != a.end()) {
    auto val = std::move(it->second);
    a.erase(it);                                   // 析构 + 释放节点
    a.emplace(new_key, std::move(val));            // 分配节点 + 构造
}

// C++17：extract + 改 key + insert
auto node = a.extract(old_key);
node.key() = new_key;
a.insert(std::move(node));                         // 节点内存直接复用
```
`extract` 后元素始终活着，`value` 的引用/指针不失效，节点内存被复用，只有必要的重新平衡开销。

</details>
