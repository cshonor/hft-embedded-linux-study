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
