# 2.2 switch 带初始化与最佳实践

> 第 2 章 if/switch 带初始化 · 上一节：[2.1 if 带初始化](01-if带初始化.md)

## 这节讲什么

switch 也能带初始化，用法和 if-init 类似。此外，本节总结 if/switch 带初始化的最佳实践和常见陷阱。

## switch 带初始化

```cpp
switch (init-statement; condition) {
    case 1: ...; break;
    case 2: ...; break;
    default: ...;
}
```

### 示例

```cpp
// 根据计算结果分发
switch (auto status = process_tick(); status) {
    case Status::OK:
        commit();
        break;
    case Status::REJECTED:
        log_reject();
        break;
    case Status::TIMEOUT:
        retry();
        break;
    default:
        unknown(status);
}

// status 在 switch 块内可见，出 switch 后不可见
```

### 实用场景

```cpp
// 1. 读枚举字段后分发
switch (auto type = msg.header().type; type) {
    case MsgType::NEW: handle_new(msg); break;
    case MsgType::CANCEL: handle_cancel(msg); break;
    case MsgType::MODIFY: handle_modify(msg); break;
}

// 2. 计算 + 分发
switch (auto bucket = latency / 100; bucket) {
    case 0:  // 0-99us
        stats.fast++;
        break;
    case 1:  // 100-199us
        stats.normal++;
        break;
    default: // 200us+
        stats.slow++;
        break;
}
```

## 最佳实践

### 1. 默认用 if-init 代替裸变量声明

```cpp
// 不好：it 泄漏
auto it = m.find(key);
if (it != m.end()) { use(it->second); }

// 好：it 限制在 if 内
if (auto it = m.find(key); it != m.end()) { use(it->second); }
```

### 2. 锁 + 条件检查用 if-init

```cpp
// 好：锁和检查在一行
if (std::lock_guard lk(mtx); condition) {
    // 临界区
}

// 不好：锁在外层，容易忘记
std::lock_guard lk(mtx);
if (condition) {
    // 临界区
}
```

### 3. 不要在 init 里写复杂逻辑

```cpp
// 不好：init 太复杂
if (auto x = [&]{ /* 20 行逻辑 */ return compute(); }(); x > 0) { ... }

// 好：init 简单，逻辑放在 if 体内
if (auto x = compute(); x > 0) {
    /* 20 行逻辑 */
}
```

### 4. if-init + 结构化绑定

```cpp
// try_emplace 返回 pair<iterator, bool>
if (auto [it, ok] = m.try_emplace(key, val); ok) {
    // 新插入
} else {
    it->second = val;  // 覆盖
}
```

## 常见陷阱

### 陷阱 1：忘记 init 后面的分号

```cpp
// 语法错误：缺少分号
if (auto it = m.find(key) it != m.end()) { ... }

// 正确
if (auto it = m.find(key); it != m.end()) { ... }
```

### 陷阱 2：在 else 分支误以为 init 失败了

```cpp
if (auto it = m.find(key); it != m.end()) {
    use(it->second);
} else {
    // it 仍然有效，但 it == m.end()
    // 不是"没找到所以 it 无效"
    // it 可以用于调试：std::cout << it->first; // UB! it == end()
}
```

### 陷阱 3：在 init 中声明多个变量

```cpp
// 只能有一个声明语句
if (auto it = m.find(key), end = m.end(); it != end) { ... }
// 这在语法上可以（逗号分隔的声明），但不推荐——可读性差

// 推荐：分开写
auto it = m.find(key);
auto end = m.end();
if (it != end) { ... }
```

## HFT 关联

```cpp
// 延迟分桶统计
switch (auto us = elapsed_us(); us / 10) {
    case 0: stats.us_0_9++; break;
    case 1: stats.us_10_19++; break;
    case 2: stats.us_20_29++; break;
    default: stats.us_30_plus++; break;
}
// us 不泄漏到外层

// 消息类型分发
switch (auto t = parse_header(buf); t) {
    case MsgType::ORDER: handle_order(buf); break;
    case MsgType::TRADE: handle_trade(buf); break;
    default: stats.unknown++;
}
```

## 小结

| 特性 | if-init | switch-init |
|------|---------|-------------|
| 语法 | `if (init; cond) { ... }` | `switch (init; cond) { case ... }` |
| init 作用域 | if-else 块 | switch 块 |
| 典型用法 | 查找/锁/optional | 枚举分发/计算分桶 |

---

← [上一节](01-if带初始化.md) · [本章导读](./README.md)
