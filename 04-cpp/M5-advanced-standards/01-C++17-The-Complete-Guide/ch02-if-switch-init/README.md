# 第 2 章 if/switch 带初始化

**if and switch with Initialization**

## 本章讲什么

C++17 让 `if` 和 `switch` 的条件里可以带一个**初始化语句**，把临时变量的作用域限制在 if/else 块内，避免变量泄漏到外层。

## 要点

### 语法

```cpp
// if 带初始化
if (auto it = m.find(key); it != m.end()) {
    use(it->second);
}   // it 在这里出作用域，不污染外层

// switch 带初始化
switch (auto x = compute(); x) {
    case 1: ...; break;
    case 2: ...; break;
}
```

形式：`if (init; condition)` / `switch (init; condition)`。init 可以是声明、表达式语句。

### 解决的老问题

```cpp
// C++14：it 泄漏到外层
auto it = m.find(key);
if (it != m.end()) { use(it->second); }
// it 还在作用域，可能被误用

// C++17：it 限制在 if/else 内
if (auto it = m.find(key); it != m.end()) {
    use(it->second);
} else {
    // it 也可见（整个 if/else 块）
}
```

### 典型用法

```cpp
// 1. 锁 + 检查
if (std::lock_guard lk(m); !queue.empty()) {
    process(queue.front());
}

// 2. 资源获取 + 检查
if (auto f = open(path); f.good()) {
    read(f);
}

// 3. optional 解包
if (auto opt = lookup(key); opt) {
    use(*opt);
}
```

## HFT 关联

- **锁+检查合并**：`if (std::lock_guard lk(m); !q.empty())` 把锁和检查写在同一行，作用域清晰，避免忘解锁。
- **optional 查找**：行情字典查找 `if (auto it = sym2id.find(sym); it != sym2id.end())` 限制迭代器作用域。
- **作用域收紧减少误用**：HFT 代码里临时变量多，收紧作用域降低误改风险。
- **零开销**：纯语法糖，编译后和分开写等价。

## 自测题

1. `if (init; cond)` 中 init 的作用域是什么？else 分支能看到 init 吗？
2. if 带初始化解决了什么老问题？
3. 用 if 带初始化写一个"加锁 + 检查队列非空"的例子。
4. switch 带初始化有什么用？举例。
5. 这个特性有运行时开销吗？

## 代码自测

### Q1: if-init 语法
```cpp
// C++14
auto it = map.find(key);
if (it != map.end()) { use(it->second); }

// C++17
if (auto it = map.find(key); it != map.end()) { use(it->second); }
```
> if-init 有什么好处？`it` 的作用域到哪里结束？

<details>
<summary>答案与复习指引</summary>

**好处**：
1. **作用域限制**：`it` 只在 if/else 块内可见，不污染外部作用域
2. **防误用**：避免 if 外面意外引用 `it`（它可能未初始化）
3. **更简洁**：一行完成"获取+检查"

**作用域**：`it` 在整个 if-else 语句（含 else 分支）内可见，if-else 结束后销毁。

也适用于 switch：
```cpp
switch (auto x = compute(); x) {
    case 1: ...; break;
    case 2: ...; break;
}
```

**复习：** → [if-init](./README.md)
</details>
