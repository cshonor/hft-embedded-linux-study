# Item 25：对万能引用用 std::forward，对右值引用用 std::move

> 第 5 章 · Item 25 · 上一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 为什么要学这个（先建立直觉）

C 没有引用的概念——所有传参要么按值（拷贝），要么传指针。C++ 有左值引用、右值引用、万能引用，混用 `move` 和 `forward` 会导致严重 bug。

核心场景：你在模板函数里收到一个参数 `x`，要把它传给另一个函数。问题是——`x` 在函数内部是左值（有名字的就是左值），不管它声明时是 `T&&` 还是 `T&`。你需要用 `forward` 或 `move` 恢复它的原始左右值性。

```cpp
// 万能引用：x 可能是左值也可能是右值
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 保留原始性

// 右值引用：x 一定是右值（但函数内是左值！）
void take(Widget&& w) { target(std::move(w)); }  // 无条件转右值
```

**致命错误：** 对万能引用用 `std::move`——如果实参是左值，`move` 会无条件搬走它，调用方的左值被掏空。

---

## 这节讲什么

混用 `move` 和 `forward` 会导致意外拷贝或意外移动。规则很简单：万能引用用 `forward`，右值引用用 `move`。

---

## 核心规则

### 万能引用 → forward

```cpp
template<class T>
void set(T&& x) { target(std::forward<T>(x)); }  // 万能引用 → forward

std::string s = "hello";
set(s);               // T = string& → forward 保留左值 → target(string&)
set(std::string("x")); // T = string → forward 转右值 → target(string&&)
```

### 右值引用 → move

```cpp
void take(Widget&& w) { target(std::move(w)); }  // 右值引用 → move
// w 在函数内是左值（有名字），但声明为右值引用 → 用 move 恢复右值性
```

### 致命错误：对万能引用用 move

```cpp
template<class T>
void bad(T&& x) { target(std::move(x)); }  // 危险！

std::string s = "important data";
bad(s);  // T = string& → x 是左值引用
// std::move(x) 无条件转右值 → s 被掏空！
std::cout << s;  // s 是空字符串！
```

### 最后一次使用可以 move

```cpp
template<class T>
void set_two(T&& x) {
    target1(x);                    // 第一次使用：不 move
    target2(std::forward<T>(x));   // 最后一次：forward（或 move 如果是右值引用）
}
```

---

## 常见错误（新手踩坑）

**错误 1：万能引用用了 move**
```cpp
template<class T>
void add(T&& item) {
    storage.push_back(std::move(item));  // 左值实参被掏空！
}
std::string s = "data";
add(s);  // s 被掏空！
```
**修正：** `storage.push_back(std::forward<T>(item));`

**错误 2：右值引用用了 forward**
```cpp
void process(Widget&& w) {
    target(std::forward<Widget>(w));  // 能用但不直观
    // 右值引用一定是右值，用 move 更清晰
}
```
**修正：** `target(std::move(w));`——语义更明确。

**错误 3：return 时用 move 阻止 NRVO**
```cpp
template<class T>
T make() {
    T result;
    return std::move(result);  // 阻止 NRVO！
}
```
**修正：** `return result;`——让编译器做返回值优化。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 参数传递 | 按值/指针 | 值/引用/右值引用/万能引用 | C++ 有引用和移动语义 |
| "搬走"标记 | 手动交换+置空 | `move`（右值引用）/ `forward`（万能引用） | 类型安全 |
| 转发 | 不适用 | `forward<T>` 保留左右值性 | 完美转发 |

**口诀：** 万能引用配 `forward`，右值引用配 `move`。不确定是哪种引用？回到 Item 24 的判断标准（有 `T&&` + 类型推导 = 万能引用）。

---

## HFT 关联

- **回调转发**：`template<class F> void onEvent(F&& cb) { storage.push_back(std::forward<F>(cb)); }` 保留 cb 的左右值性，避免不必要的拷贝。
- **订单构造**：`template<class... Args> void emplace_order(Args&&... args) { orders.emplace_back(std::forward<Args>(args)...); }` 完美转发构造参数。
- **配置传递**：策略配置用万能引用接收，`forward` 到内部存储——避免拷贝大配置对象。

---

## 自测题

1. 对万能引用用 `std::move` 为什么危险？
2. 什么时候用 `std::move`？什么时候用 `std::forward`？
3. `std::forward<T>(x)` 为什么必须显式传 `T`？
4. 函数内右值引用参数为什么是左值？需要用什么恢复？
5. 下面代码有什么问题？
```cpp
template<class T>
void store(T&& x) {
    cache = std::move(x);
}
std::string config = "important";
store(config);
std::cout << config;
```

---

## 参考与延伸

- 下一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)
- 回到：[第 5 章](README.md)
