# 第 26 章 容器与 string 扩展

**Container and String Extensions**

## 本章讲什么

C++17 给 STL 容器和 string 加了些实用小改进：`try_emplace`/`insert_or_assign`、`extract`/`merge`、`node_type`、`string_view` 互操作、`data()` 非成员版。

## 要点

### map 的 `try_emplace` / `insert_or_assign`

```cpp
std::map<std::string, Obj> m;

// C++14 emplace 的问题：key 已存在时仍构造 value（浪费）
m.emplace("k", Obj(...));   // 构造了 Obj，但 key 已存在则丢弃

// C++17 try_emplace：key 已存在时不构造 value
m.try_emplace("k", args...);   // key 存在则什么都不做，不构造 Obj

// insert_or_assign：key 存在则赋值，不存在则插入
m.insert_or_assign("k", Obj(...));   // 总是更新/插入
```

`try_emplace` 的价值：value 构造有副作用或开销时（如分配资源），key 已存在能避免构造。

### `extract` / `merge`：节点转移

```cpp
std::map<int, std::string> a, b;
a[1] = "one";

// extract：取出节点（不拷贝、不分配）
auto node = a.extract(1);   // node 持有 a 中的节点所有权
node.key() = 2;             // 可改 key（map 独有）
b.insert(std::move(node));  // 插入 b，零拷贝

// merge：批量转移
b.merge(a);   // 把 a 的所有节点移到 b，零拷贝
```

节点提取让 map/set 之间转移元素**零拷贝、零分配**——适合大对象的重组。

### `node_type`

`extract` 返回 `node_type`，是独占所有权的节点句柄：
- 空（`empty()`）时析构无副作用。
- 可移动不可拷贝。
- map 的 node 可改 key，set 的不可改。

### `string_view` 与 string 互操作

```cpp
std::string_view sv = "hello";
std::string s{sv};            // string_view → string（拷贝）
std::string_view sv2 = s;     // string → string_view（零拷贝）

// string 的构造/append/replace 接受 string_view
s.append(sv);
s.replace(0, 2, sv);
```

### `nonmember data()` / `size()` / `empty()`

```cpp
std::string s = "hi";
std::vector<int> v = {1,2};
int arr[3] = {1,2,3};

std::data(s);   // s.data()
std::data(v);   // v.data()
std::data(arr); // arr
```

### `contiguous` 迭代器（概念上）

C++17 起 `vector`/`string`/`array` 的迭代器被定义为 **contiguous iterator**（C++20 正式概念），保证底层连续，可和 C API 互操作。

## HFT 关联

- **`try_emplace` 避免无用构造**：合约表 `try_emplace(sym, Contract{...})` 合约已存在时不构造新对象，避免分配。
- **`extract`/`merge` 重组订单簿**：订单簿重建时用 `extract` 取节点 + `insert` 到新 map，零拷贝转移大订单对象。
- **`insert_or_assign` 配置热更新**：策略参数表 `insert_or_assign(key, new_val)` 存在则更新、不存在则插入，一行搞定。
- **`string_view` 互操作**：日志库内部存 `string`，对外接口接受 `string_view`，无拷贝转换。
- **`data()` 与 C API**：`std::data(buf)` 传给 `memcpy`/`send`，泛型代码对数组和容器统一。
- **节点改 key 重映射**：合约改名时 `extract` + 改 key + `insert`，比 erase+insert 少一次析构构造。

## 自测题

1. `try_emplace` 相比 `emplace` 解决了什么问题？
2. `extract` + `insert` 相比 `erase` + `insert` 有什么优势？
3. `node_type` 能改 key 吗？map 和 set 的区别？
4. `insert_or_assign` 和 `try_emplace` 的语义区别？
5. HFT 订单簿重组如何用 `extract`/`merge` 零拷贝转移？
