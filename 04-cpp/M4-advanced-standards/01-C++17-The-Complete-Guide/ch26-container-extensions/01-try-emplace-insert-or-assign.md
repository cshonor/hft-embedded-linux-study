# try_emplace / insert_or_assign

## emplace 的问题

```cpp
std::map<std::string, Obj> m;

// emplace：即使 key 已存在，也会构造 value（然后丢弃）
m.emplace("k", Obj("expensive"));  // 如果 "k" 存在，Obj("expensive") 白构造了
```

C++14 的 `emplace` 先构造参数，再检查 key 是否存在——如果存在，构造的 value 被丢弃。如果 `Obj` 构造有副作用（分配资源、打印日志），就浪费了。

## try_emplace

```cpp
// C++17：key 已存在时不构造 value
auto [it, inserted] = m.try_emplace("k", "expensive_args");

if (inserted) {
    // key 不存在，新插入，Obj 被构造
} else {
    // key 已存在，Obj 没有被构造（无浪费）
    // it->second 保持原值不变
}

// try_emplace 的参数直接转发给 value 的构造函数
// 只有在真正需要插入时才构造
```

**关键**：`try_emplace` 只在 key 不存在时才构造 value。参数以 `args...` 形式传入，不是先构造好再传。

## insert_or_assign

```cpp
// insert_or_assign：存在则赋值，不存在则插入
auto [it, inserted] = m.insert_or_assign("k", Obj("new"));

if (inserted) {
    // key 不存在，新插入
} else {
    // key 已存在，旧值被赋值为 Obj("new")
}

// 等价于：
// m["k"] = Obj("new");  但 operator[] 默认构造再赋值，insert_or_assign 直接赋值
```

## 对比表

| 方法 | key 存在时 | key 不存在时 | value 构造时机 |
|------|-----------|-------------|---------------|
| `emplace` | 丢弃新 value（已构造） | 插入 | 总是构造 |
| `try_emplace` | 不构造、不修改 | 插入 | 仅插入时 |
| `insert_or_assign` | 赋值 | 插入 | 总是构造 |
| `operator[]` | 默认构造+赋值 | 默认构造插入 | 总是构造 |

## 实际应用

```cpp
// 1. 合约表：合约已存在不重复构造
std::map<std::string, Contract> contracts;
contracts.try_emplace("AAPL", "AAPL", exchange_id, tick_size);
// "AAPL" 已存在时不构造 Contract

// 2. 配置热更新：存在则更新
std::map<std::string, Config> configs;
configs.insert_or_assign("strategy_1", new_config);
// 存在则更新为新配置，不存在则插入

// 3. 延迟构造：value 构造昂贵时省去无用构造
std::map<int, BigResource> cache;
cache.try_emplace(key, expensive_factory());
// key 存在时 expensive_factory() 不被调用（try_emplace 转发参数，不先求值）
// 注意：实际上 try_emplace 的参数在调用前会被求值！
// 正确写法：
cache.try_emplace(key, [&]() { return expensive_factory(); });
// 不对——try_emplace 仍然先求值参数
// 真正延迟：用 lambda 或 emplace(piecewise_construct, ...)
```

> **注意**：`try_emplace` 的参数会先被求值再传入。真正的延迟构造需要用 `try_emplace(key, arg1, arg2)` 把构造参数分开传，让 map 内部转发给构造函数，而不是先构造好对象再传入。

## 自测题

1. `emplace` 在 key 已存在时有什么浪费？
2. `try_emplace` 在 key 存在时是否构造 value？
3. `insert_or_assign` 和 `operator[]` 的区别？
4. `try_emplace` 的参数是先求值还是延迟构造？如何实现真正延迟？
5. 配置热更新用哪个方法？合约表初始化用哪个？
